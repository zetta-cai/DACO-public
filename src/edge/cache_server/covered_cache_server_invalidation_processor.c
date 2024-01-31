#include "edge/cache_server/covered_cache_server_invalidation_processor.h"

#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CoveredCacheServerInvalidationProcessor::kClassName("CoveredCacheServerInvalidationProcessor");

    CoveredCacheServerInvalidationProcessor::CoveredCacheServerInvalidationProcessor(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr) : CacheServerInvalidationProcessorBase(cache_server_invalidation_processor_param_ptr)
    {
        assert(cache_server_invalidation_processor_param_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_invalidation_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-invalidation-processor";
        instance_name_ = oss.str();
    }

    CoveredCacheServerInvalidationProcessor::~CoveredCacheServerInvalidationProcessor()
    {}

    void CoveredCacheServerInvalidationProcessor::processReqForInvalidation_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CacheWrapper* tmp_cache_wrapper_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        assert(control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest);

        // Get key from request message
        //uint32_t source_edge_idx = 0;
        const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
        Key tmp_key = covered_invalidation_request_ptr->getKey();
        //source_edge_idx = covered_invalidation_request_ptr->getSourceIndex();

        // Invalidate cached object in local edge cache
        bool is_local_cached = tmp_cache_wrapper_ptr->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            tmp_cache_wrapper_ptr->invalidateKeyForLocalCachedObject(tmp_key);
        }

        // // Victim synchronization
        // const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
        // const VictimSyncset& neighbor_victim_syncset = covered_invalidation_request_ptr->getVictimSyncsetRef();
        // UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        // edge_wrapper_ptr_->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    MessageBase* CoveredCacheServerInvalidationProcessor::getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_invalidation_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        //CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqPublicSourceAddr(); // NOTE: cross-edge communication for cache invalidation uses public IP address
        //const uint64_t cache_margin_bytes = tmp_edge_wrapper_ptr->getCacheMarginBytes();

        assert(control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest);

        Key tmp_key;
        bool skip_propagation_latency = false;
        MessageBase* invalidation_response_ptr = NULL;
        
        const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
        tmp_key = covered_invalidation_request_ptr->getKey();
        skip_propagation_latency = covered_invalidation_request_ptr->isSkipPropagationLatency();

        // // Prepare victim syncset for piggybacking-based victim synchronization
        // const uint32_t dst_edge_idx_for_compression = covered_invalidation_request_ptr->getSourceIndex();
        // VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, cache_margin_bytes);

        // Prepare invalidation response
        // invalidation_response_ptr = new CoveredInvalidationResponse(tmp_key, victim_syncset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        invalidation_response_ptr = new CoveredInvalidationResponse(tmp_key, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(invalidation_response_ptr != NULL);

        return invalidation_response_ptr;
    }
}