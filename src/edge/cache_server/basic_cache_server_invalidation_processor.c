#include "edge/cache_server/basic_cache_server_invalidation_processor.h"

#include "cache/basic_cache_custom_func_param.h"
#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicCacheServerInvalidationProcessor::kClassName("BasicCacheServerInvalidationProcessor");

    BasicCacheServerInvalidationProcessor::BasicCacheServerInvalidationProcessor(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr) : CacheServerInvalidationProcessorBase(cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_invalidation_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-invalidation-processor";
        instance_name_ = oss.str();
    }

    BasicCacheServerInvalidationProcessor::~BasicCacheServerInvalidationProcessor()
    {}

    void BasicCacheServerInvalidationProcessor::processReqForInvalidation_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CacheWrapper* tmp_cache_wrapper_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        assert(control_request_ptr != NULL);
        const MessageType message_type = control_request_ptr->getMessageType();
        uint32_t source_edge_idx = control_request_ptr->getSourceIndex();

        // Get key from request message
        Key tmp_key;
        if (message_type == MessageType::kInvalidationRequest)
        {
            const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
            tmp_key = invalidation_request_ptr->getKey();
        }
        else if (message_type == MessageType::kBestGuessInvalidationRequest)
        {
            const BestGuessInvalidationRequest* const best_guess_invalidation_request_ptr = static_cast<const BestGuessInvalidationRequest*>(control_request_ptr);
            tmp_key = best_guess_invalidation_request_ptr->getKey();

            // Vtime synchronization
            const BestGuessSyncinfo syncinfo = best_guess_invalidation_request_ptr->getSyncinfo();
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerInvalidationProcessor::processReqForInvalidation_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Invalidate cached object in local edge cache
        bool is_local_cached = tmp_cache_wrapper_ptr->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            tmp_cache_wrapper_ptr->invalidateKeyForLocalCachedObject(tmp_key);
        }

        return;
    }

    MessageBase* BasicCacheServerInvalidationProcessor::getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqPublicSourceAddr(); // NOTE: cross-edge communication for cache invalidation uses public IP address

        assert(control_request_ptr != NULL);
        const MessageType message_type = control_request_ptr->getMessageType();

        MessageBase* invalidation_response_ptr = NULL;
        if (message_type == MessageType::kInvalidationRequest)
        {
            const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
            const Key tmp_key = invalidation_request_ptr->getKey();
            const bool skip_propagation_latency = invalidation_request_ptr->isSkipPropagationLatency();

            // Prepare invalidation response
            invalidation_response_ptr = new InvalidationResponse(tmp_key, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (message_type == MessageType::kBestGuessInvalidationRequest)
        {
            const BestGuessInvalidationRequest* const best_guess_invalidation_request_ptr = static_cast<const BestGuessInvalidationRequest*>(control_request_ptr);
            const Key tmp_key = best_guess_invalidation_request_ptr->getKey();
            const bool skip_propagation_latency = best_guess_invalidation_request_ptr->isSkipPropagationLatency();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            // Prepare invalidation response
            invalidation_response_ptr = new BestGuessInvalidationResponse(tmp_key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerInvalidationProcessor::getRspForInvalidation_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(invalidation_response_ptr != NULL);

        return invalidation_response_ptr;
    }
}