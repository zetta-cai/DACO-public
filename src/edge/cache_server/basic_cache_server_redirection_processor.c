#include "edge/cache_server/basic_cache_server_redirection_processor.h"

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
        assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);

        // Get key from redirected get requests
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();

        // Access local edge cache for redirected get request
        is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, is_redirected, value);
        is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);
        
        return;
    }

    MessageBase* BasicCacheServerRedirectionProcessor::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqPublicSourceAddr(); // NOTE: cross-edge communication for request redirection uses public IP address

        MessageBase* redirected_get_response_ptr = NULL;
        assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);

        // Get key from redirected get request
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();

        // Prepare redirected get response
        redirected_get_response_ptr = new RedirectedGetResponse(tmp_key, value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(redirected_get_response_ptr != NULL);

        return redirected_get_response_ptr;
    }
}