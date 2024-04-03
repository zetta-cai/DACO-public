#include "edge/cache_server/cache_server_invalidation_processor_base.h"

#include <assert.h>

#include "edge/cache_server/basic_cache_server_invalidation_processor.h"
#include "edge/cache_server/covered_cache_server_invalidation_processor.h"
#include "cache/covered_local_cache.h"
#include "common/config.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerInvalidationProcessorBase::kClassName = "CacheServerInvalidationProcessorBase";

    void* CacheServerInvalidationProcessorBase::launchCacheServerInvalidationProcessor(void* cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);

        CacheServerProcessorParam* tmp_cache_server_invalidation_processor_param_ptr = (CacheServerProcessorParam*)cache_server_invalidation_processor_param_ptr;
        const std::string cache_name = tmp_cache_server_invalidation_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getCacheName();
        CacheServerInvalidationProcessorBase* cache_server_invalidation_processor_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            cache_server_invalidation_processor_ptr = new CoveredCacheServerInvalidationProcessor(tmp_cache_server_invalidation_processor_param_ptr);
        }
        else
        {
            cache_server_invalidation_processor_ptr = new BasicCacheServerInvalidationProcessor(tmp_cache_server_invalidation_processor_param_ptr);
        }
        assert(cache_server_invalidation_processor_ptr != NULL);
        cache_server_invalidation_processor_ptr->start();

        delete cache_server_invalidation_processor_ptr;
        cache_server_invalidation_processor_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerInvalidationProcessorBase::CacheServerInvalidationProcessorBase(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr) : cache_server_invalidation_processor_param_ptr_(cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_invalidation_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-invalidation-processor";
        base_instance_name_ = oss.str();
    }

    CacheServerInvalidationProcessorBase::~CacheServerInvalidationProcessorBase()
    {
        // NOTE: no need to release cache_server_invalidation_processor_param_ptr_, which will be released outside CacheServerInvalidationProcessorBase (by CacheServer)
    }

    void CacheServerInvalidationProcessorBase::start()
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Notify edge cache server that the current edge cache server invalidation processor has finished initialization
        cache_server_invalidation_processor_param_ptr_->markFinishInitialization();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_invalidation_processor_param_ptr_->pop(tmp_cache_server_item, (void*)tmp_edge_wrapper_ptr);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* control_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(control_request_ptr != NULL);

                if (control_request_ptr->getMessageType() == MessageType::kInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kBestGuessInvalidationRequest) // Invalidation request
                {
                    NetworkAddr recvrsp_dst_addr = control_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processInvalidationRequest_(control_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
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

    bool CacheServerInvalidationProcessorBase::processInvalidationRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest || control_request_ptr->getMessageType() == MessageType::kBestGuessInvalidationRequest);

        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received invalidation request
        uint32_t cross_edge_invalidation_req_bandwidth_bytes = control_request_ptr->getMsgBandwidthSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_invalidation_req_bandwidth_bytes, 0, 0, 1, 0, control_request_ptr->getMessageType(), control_request_ptr->getVictimSyncsetBytes()));

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

    void CacheServerInvalidationProcessorBase::checkPointers_() const
    {
        assert(cache_server_invalidation_processor_param_ptr_ != NULL);

        return;
    }
}