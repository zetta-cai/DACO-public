#include "edge/cache_server/cache_server_redirection_processor_base.h"

#include <assert.h>

#include "common/config.h"
#include "edge/cache_server/basic_cache_server_redirection_processor.h"
#include "edge/cache_server/covered_cache_server_redirection_processor.h"

namespace covered
{
    const std::string CacheServerRedirectionProcessorBase::kClassName = "CacheServerRedirectionProcessorBase";

    void* CacheServerRedirectionProcessorBase::launchCacheServerRedirectionProcessor(void* cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);

        CacheServerProcessorParam* tmp_cache_server_processor_param_ptr = (CacheServerProcessorParam*)cache_server_redirection_processor_param_ptr;
        const std::string cache_name = tmp_cache_server_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getCacheName();
        CacheServerRedirectionProcessorBase* cache_server_redirection_processor_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            cache_server_redirection_processor_ptr = new CoveredCacheServerRedirectionProcessor(tmp_cache_server_processor_param_ptr);
        }
        else
        {
            cache_server_redirection_processor_ptr = new BasicCacheServerRedirectionProcessor(tmp_cache_server_processor_param_ptr);
        }
        assert(cache_server_redirection_processor_ptr != NULL);
        cache_server_redirection_processor_ptr->start();

        delete cache_server_redirection_processor_ptr;
        cache_server_redirection_processor_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerRedirectionProcessorBase::CacheServerRedirectionProcessorBase(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr) : cache_server_redirection_processor_param_ptr_(cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_redirection_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-redirection-processor";
        base_instance_name_ = oss.str();
    }

    CacheServerRedirectionProcessorBase::~CacheServerRedirectionProcessorBase()
    {
        // NOTE: no need to release cache_server_redirection_processor_param_ptr_, which will be released outside CacheServerRedirectionProcessorBase (by CacheServer)
    }

    void CacheServerRedirectionProcessorBase::start()
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_redirection_processor_param_ptr_->pop(tmp_cache_server_item, (void*)tmp_edge_wrapper_ptr);
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
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
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

    bool CacheServerRedirectionProcessorBase::processRedirectedDataRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr)
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
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool CacheServerRedirectionProcessorBase::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        assert(redirected_request_ptr != NULL);
        assert(recvrsp_dst_addr.isValidAddr());

        // Get key and value from redirected request if any
        /*assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received redirected get request
        uint32_t cross_edge_redirected_get_req_bandwidth_bytes = redirected_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_req_bandwidth_bytes, 0, 0, 1, 0));

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

    void CacheServerRedirectionProcessorBase::checkPointers_() const
    {
        assert(cache_server_redirection_processor_param_ptr_ != NULL);

        return;
    }
}