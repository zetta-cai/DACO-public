#include "edge/invalidation_server/covered_invalidation_server.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredInvalidationServer::kClassName("CoveredInvalidationServer");

    CoveredInvalidationServer::CoveredInvalidationServer(EdgeWrapper* edge_wrapper_ptr) : InvalidationServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate CoveredInvalidationServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredInvalidationServer::~CoveredInvalidationServer() {}

    void CoveredInvalidationServer::processReqForInvalidation_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest);
        // CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
        const uint32_t source_edge_idx = covered_invalidation_request_ptr->getSourceIndex();
        Key tmp_key = covered_invalidation_request_ptr->getKey();

        // Invalidate cached object in local edge cache
        bool is_local_cached = edge_wrapper_ptr_->getEdgeCachePtr()->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            edge_wrapper_ptr_->getEdgeCachePtr()->invalidateKeyForLocalCachedObject(tmp_key);
        }

        // Victim synchronization
        const VictimSyncset& neighbor_victim_syncset = covered_invalidation_request_ptr->getVictimSyncsetRef();
        edge_wrapper_ptr_->updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);

        return;
    }

    MessageBase* CoveredInvalidationServer::getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list)
    {
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest);
        CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(control_request_ptr);
        Key tmp_key = covered_invalidation_request_ptr->getKey();
        const bool skip_propagation_latency = covered_invalidation_request_ptr->isSkipPropagationLatency();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = covered_invalidation_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        // Prepare invalidation response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_invalidation_response_ptr = new CoveredInvalidationResponse(tmp_key, victim_syncset, edge_idx, edge_invalidation_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_invalidation_response_ptr != NULL);

        return covered_invalidation_response_ptr;
    }
}