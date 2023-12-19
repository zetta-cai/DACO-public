#include "edge/cache_server/cache_server_redirection_processor.h"

#include <assert.h>

#include "common/config.h"
#include "message/data_message.h"

namespace covered
{
    const std::string CacheServerRedirectionProcessor::kClassName = "CacheServerRedirectionProcessor";

    void* CacheServerRedirectionProcessor::launchCacheServerRedirectionProcessor(void* cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);

        CacheServerRedirectionProcessor cache_server_redirection_processor((CacheServerProcessorParam*)cache_server_redirection_processor_param_ptr);
        cache_server_redirection_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerRedirectionProcessor::CacheServerRedirectionProcessor(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr) : cache_server_redirection_processor_param_ptr_(cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_redirection_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-redirection-processor";
        instance_name_ = oss.str();
    }

    CacheServerRedirectionProcessor::~CacheServerRedirectionProcessor()
    {
        // NOTE: no need to release cache_server_redirection_processor_param_ptr_, which will be released outside CacheServerRedirectionProcessor (by CacheServer)
    }

    void CacheServerRedirectionProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_redirection_processor_param_ptr_->getCacheServerItemBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isRedirectedDataRequest()) // Redirected data request
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // Cache server worker addr for foreground request redirection or beacon server addr for background request redirection
                    is_finish = processRedirectedDataRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server redirection processor thread
                assert(data_request_ptr != NULL);
                delete data_request_ptr;
                data_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    // Process redirected requests

    bool CacheServerRedirectionProcessor::processRedirectedDataRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedDataRequest());

        bool is_finish = false; // Mark if local edge node is finished

        MessageType message_type = redirected_request_ptr->getMessageType();
        if (message_type == MessageType::kRedirectedGetRequest || message_type == MessageType::kCoveredRedirectedGetRequest || message_type == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            is_finish = processRedirectedGetRequest_(redirected_request_ptr, recvrsp_dst_addr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for processRedirectedRequest_()!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool CacheServerRedirectionProcessor::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        assert(redirected_request_ptr != NULL);
        assert(recvrsp_dst_addr.isValidAddr());

        // Get key and value from redirected request if any
        /*assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received redirected get request
        uint32_t cross_edge_redirected_get_req_bandwidth_bytes = redirected_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_req_bandwidth_bytes, 0));

        struct timespec target_get_local_cache_start_timestamp = Util::getCurrentTimespec();

        // Access local edge cache for cooperative edge caching (current edge node is the target edge node)
        Value tmp_value;
        bool is_cooperaitve_cached = false;
        bool is_cooperative_cached_and_valid = false;
        processReqForRedirectedGet_(redirected_request_ptr, tmp_value, is_cooperaitve_cached, is_cooperative_cached_and_valid);

        // Set hitflag accordingly
        Hitflag hitflag = Hitflag::kGlobalMiss;
        if (is_cooperative_cached_and_valid) // cached and valid
        {
            hitflag = Hitflag::kCooperativeHit;
        }
        else // not cached or invalid
        {
            if (is_cooperaitve_cached) // cached and invalid
            {
                hitflag = Hitflag::kCooperativeInvalid;
            }
        }

        struct timespec target_get_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t target_get_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(target_get_local_cache_end_timestamp, target_get_local_cache_start_timestamp));
        bool is_background_request = redirected_request_ptr->isBackgroundRequest();
        if (is_background_request)
        {
            event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME, target_get_local_cache_latency_us);
        }
        else
        {
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME, target_get_local_cache_latency_us);
        }

        // NOTE: no need to perform recursive cooperative edge caching (current edge node is already the target edge node for cooperative edge caching)
        // NOTE: no need to access cloud to get data, which will be performed by the closest edge node

        // Prepare RedirectedGetResponse for the closest edge node
        MessageBase* redirected_get_response_ptr = getRspForRedirectedGet_(redirected_request_ptr, tmp_value, hitflag, total_bandwidth_usage, event_list);
        assert(redirected_get_response_ptr != NULL);

        // Push the redirected response message into edge-to-client propagation simulator to cache server worker in the closest edge node
        tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(redirected_get_response_ptr, recvrsp_dst_addr);

        // NOTE: redirected_get_response_ptr will be released by edge-to-client propagation simulator
        redirected_get_response_ptr = NULL;

        return is_finish;
    }

    void CacheServerRedirectionProcessor::processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        const bool is_redirected = true;
        if (tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
            // CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

            // Get key and victim syncset from redirected get request
            Key tmp_key;
            uint32_t source_edge_idx = redirected_request_ptr->getSourceIndex();
            VictimSyncset neighbor_victim_syncset;
            if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
            {
                const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
                tmp_key = covered_redirected_get_request_ptr->getKey();
                neighbor_victim_syncset = covered_redirected_get_request_ptr->getVictimSyncsetRef();
            }
            else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
            {
                const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
                tmp_key = covered_placement_redirected_get_request_ptr->getKey();
                neighbor_victim_syncset = covered_placement_redirected_get_request_ptr->getVictimSyncsetRef();
            }
            else
            {
                std::ostringstream oss;
                oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Access local edge cache for redirected get request
            is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, is_redirected, value);
            is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);

            // Victim synchronization
            tmp_edge_wrapper_ptr->updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
        }
        else // Baselines
        {
            assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);

            // Get key from redirected get requests
            const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
            Key tmp_key = redirected_get_request_ptr->getKey();

            // Access local edge cache for redirected get request
            is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, is_redirected, value);
            is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);
        }
        
        return;
    }

    MessageBase* CacheServerRedirectionProcessor::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqSourceAddr();

        MessageBase* redirected_get_response_ptr = NULL;
        if (tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
            CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

            // Get key (and placement edgeset) from redirected get request
            Key tmp_key;
            Edgeset tmp_placement_edgeset;
            bool skip_propagation_latency = redirected_request_ptr->isSkipPropagationLatency();
            if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
            {
                const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
                tmp_key = covered_redirected_get_request_ptr->getKey();
            }
            else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
            {
                const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
                tmp_key = covered_placement_redirected_get_request_ptr->getKey();
                tmp_placement_edgeset = covered_placement_redirected_get_request_ptr->getEdgesetRef();
            }
            else
            {
                std::ostringstream oss;
                oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Prepare victim syncset for piggybacking-based victim synchronization
            const uint32_t dst_edge_idx_for_compression = redirected_request_ptr->getSourceIndex();
            VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

            // Prepare redirected get response
            if (!redirected_request_ptr->isBackgroundRequest()) // Send back normal redirected get response
            {
                // NOTE: CoveredRedirectedGetResponse will be processed by edge cache server worker of the sender edge node
                redirected_get_response_ptr = new CoveredRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
            else // Send back redirected get response for non-blocking placement deploymeng
            {
                assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

                // NOTE: CoveredPlacementRedirectedGetResponse will be processed by edge beacon server of the sender edge node
                redirected_get_response_ptr = new CoveredPlacementRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, tmp_placement_edgeset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
        }
        else // Baselines
        {
            assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);

            // Get key from redirected get request
            const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
            Key tmp_key = redirected_get_request_ptr->getKey();
            const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();

            // Prepare redirected get response
            redirected_get_response_ptr = new RedirectedGetResponse(tmp_key, value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        assert(redirected_get_response_ptr != NULL);

        return redirected_get_response_ptr;
    }

    void CacheServerRedirectionProcessor::checkPointers_() const
    {
        assert(cache_server_redirection_processor_param_ptr_ != NULL);

        return;
    }
}