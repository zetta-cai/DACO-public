#include "edge/cache_server/basic_cache_server_redirection_processor.h"

#include "cache/basic_cache_custom_func_param.h"
#include "message/data_message.h"

namespace covered
{
    const std::string BasicCacheServerRedirectionProcessor::kClassName("BasicCacheServerRedirectionProcessor");

    BasicCacheServerRedirectionProcessor::BasicCacheServerRedirectionProcessor(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr) : CacheServerRedirectionProcessorBase(cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-redirection-processor";
        instance_name_ = oss.str();
    }

    BasicCacheServerRedirectionProcessor::~BasicCacheServerRedirectionProcessor()
    {}

    void BasicCacheServerRedirectionProcessor::processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        const bool is_redirected = true;
        const MessageType message_type = redirected_request_ptr->getMessageType();

        // Get key from redirected get requests
        Key tmp_key;
        if (message_type == MessageType::kRedirectedGetRequest)
        {
            const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = redirected_get_request_ptr->getKey();
        }
        else if (message_type == MessageType::kBestGuessRedirectedGetRequest)
        {
            const BestGuessRedirectedGetRequest* const best_guess_redirected_get_request_ptr = static_cast<const BestGuessRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = best_guess_redirected_get_request_ptr->getKey();
            BestGuessSyncinfo syncinfo = best_guess_redirected_get_request_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(best_guess_redirected_get_request_ptr->getSourceIndex(), syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicCacheServerRedirectionProcessor::processReqForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Access local edge cache for redirected get request
        bool unused_is_tracked_before_fetch_value = false;
        is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, is_redirected, value, unused_is_tracked_before_fetch_value);
        UNUSED(unused_is_tracked_before_fetch_value);
        is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);
        
        return;
    }

    MessageBase* BasicCacheServerRedirectionProcessor::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(redirected_request_ptr != NULL);
        const MessageType message_type = redirected_request_ptr->getMessageType();

        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqPublicSourceAddr(); // NOTE: cross-edge communication for request redirection uses public IP address

        MessageBase* redirected_get_response_ptr = NULL;
        if (message_type == MessageType::kRedirectedGetRequest)
        {
            // Get key from redirected get request
            const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
            Key tmp_key = redirected_get_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = redirected_get_request_ptr->getExtraCommonMsghdr();

            // Prepare redirected get response
            redirected_get_response_ptr = new RedirectedGetResponse(tmp_key, value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else if (message_type == MessageType::kBestGuessRedirectedGetRequest)
        {
            // Get key from redirected get request
            const BestGuessRedirectedGetRequest* const best_guess_redirected_get_request_ptr = static_cast<const BestGuessRedirectedGetRequest*>(redirected_request_ptr);
            Key tmp_key = best_guess_redirected_get_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = best_guess_redirected_get_request_ptr->getExtraCommonMsghdr();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            // Prepare redirected get response
            redirected_get_response_ptr = new BestGuessRedirectedGetResponse(tmp_key, value, hitflag, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicCacheServerRedirectionProcessor::getRspForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(redirected_get_response_ptr != NULL);

        return redirected_get_response_ptr;
    }
}