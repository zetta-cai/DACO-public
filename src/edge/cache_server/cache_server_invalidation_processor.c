#include "edge/cache_server/cache_server_invalidation_processor.h"

#include <assert.h>

#include "cache/covered_local_cache.h"
#include "common/config.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerInvalidationProcessor::kClassName = "CacheServerInvalidationProcessor";

    void* CacheServerInvalidationProcessor::launchCacheServerInvalidationProcessor(void* cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);

        CacheServerInvalidationProcessor cache_server_invalidation_processor((CacheServerProcessorParam*)cache_server_invalidation_processor_param_ptr);
        cache_server_invalidation_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerInvalidationProcessor::CacheServerInvalidationProcessor(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr) : cache_server_invalidation_processor_param_ptr_(cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_invalidation_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-invalidation-processor";
        instance_name_ = oss.str();
    }

    CacheServerInvalidationProcessor::~CacheServerInvalidationProcessor()
    {
        // NOTE: no need to release cache_server_invalidation_processor_param_ptr_, which will be released outside CacheServerInvalidationProcessor (by CacheServer)
    }

    void CacheServerInvalidationProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_invalidation_processor_param_ptr_->getCacheServerItemBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* control_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(control_request_ptr != NULL);

                if (control_request_ptr->getMessageType() == MessageType::kInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest) // Invalidation request
                {
                    NetworkAddr recvrsp_dst_addr = control_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processInvalidationRequest_(control_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server metadata update processor thread
                assert(control_request_ptr != NULL);
                delete control_request_ptr;
                control_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    bool CacheServerInvalidationProcessor::processInvalidationRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received invalidation request
        const CoveredMetadataUpdateRequest* const covered_metadata_update_request_ptr = static_cast<const CoveredMetadataUpdateRequest*>(control_request_ptr);
        uint32_t cross_edge_invalidation_req_bandwidth_bytes = covered_metadata_update_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_invalidation_req_bandwidth_bytes, 0));

        struct timespec invalidate_local_cache_start_timestamp = Util::getCurrentTimespec();

        // Process invalidation request
        processReqForInvalidation_(control_request_ptr);

        // Add intermediate event if with event tracking
        struct timespec invalidate_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t invalidate_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(invalidate_local_cache_end_timestamp, invalidate_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_INVALIDATION_SERVER_INVALIDATE_LOCAL_CACHE_EVENT_NAME, invalidate_local_cache_latency_us);

        // Prepare a invalidation response
        MessageBase* invalidation_response_ptr = getRspForInvalidation_(control_request_ptr, total_bandwidth_usage, event_list);

        // Push the invalidation response into edge-to-edge propagation simulator to cache server worker or beacon server
        bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(invalidation_response_ptr, recvrsp_dst_addr);
        assert(is_successful);
        
        // NOTE: invalidation_response_ptr will be released by edge-to-edge propagation simulator
        invalidation_response_ptr = NULL;

        return is_finish;
    }

    void CacheServerInvalidationProcessor::processReqForInvalidation_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CacheWrapper* tmp_cache_wrapper_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        // Get key from request message
        Key tmp_key;
        uint32_t source_edge_idx = 0;
        if (control_request_ptr->getMessageType() == MessageType::kInvalidationRequest)
        {
            const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
            tmp_key = invalidation_request_ptr->getKey();
            source_edge_idx = invalidation_request_ptr->getSourceIndex();
        }
        else if (control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest)
        {
            const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
            Key tmp_key = covered_invalidation_request_ptr->getKey();
            source_edge_idx = covered_invalidation_request_ptr->getSourceIndex();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for processReqForInvalidation_()!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Invalidate cached object in local edge cache
        bool is_local_cached = tmp_cache_wrapper_ptr->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            tmp_cache_wrapper_ptr->invalidateKeyForLocalCachedObject(tmp_key);
        }

        if (control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest)
        {
            assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);

            // Victim synchronization
            const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
            const VictimSyncset& neighbor_victim_syncset = covered_invalidation_request_ptr->getVictimSyncsetRef();
            tmp_edge_wrapper_ptr->updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
        }

        return;
    }

    MessageBase* CacheServerInvalidationProcessor::getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list)
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqSourceAddr();
        const uint64_t cache_margin_bytes = tmp_edge_wrapper_ptr->getCacheMarginBytes();

        Key tmp_key;
        bool skip_propagation_latency = false;
        MessageBase* invalidation_response_ptr = NULL;
        if (control_request_ptr->getMessageType() == MessageType::kInvalidationRequest)
        {
            const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
            tmp_key = invalidation_request_ptr->getKey();
            skip_propagation_latency = invalidation_request_ptr->isSkipPropagationLatency();

            // Prepare invalidation response
            invalidation_response_ptr = new InvalidationResponse(tmp_key, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest)
        {
            assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);

            const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
            tmp_key = covered_invalidation_request_ptr->getKey();
            skip_propagation_latency = covered_invalidation_request_ptr->isSkipPropagationLatency();

            // Prepare victim syncset for piggybacking-based victim synchronization
            const uint32_t dst_edge_idx_for_compression = covered_invalidation_request_ptr->getSourceIndex();
            VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, cache_margin_bytes);

            // Prepare invalidation response
            invalidation_response_ptr = new CoveredInvalidationResponse(tmp_key, victim_syncset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for getRspForInvalidation_()!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(invalidation_response_ptr != NULL);

        return invalidation_response_ptr;
    }

    void CacheServerInvalidationProcessor::checkPointers_() const
    {
        assert(cache_server_invalidation_processor_param_ptr_ != NULL);

        return;
    }
}