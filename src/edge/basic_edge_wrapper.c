#include "edge/basic_edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicEdgeWrapper::kClassName("BasicEdgeWrapper");

    BasicEdgeWrapper::BasicEdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt) : EdgeWrapperBase(cache_name, capacity_bytes, edge_idx, edgecnt, hash_name, local_uncached_capacity_bytes, percacheserver_workercnt, peredge_synced_victimcnt, peredge_monitored_victimsetcnt, popularity_aggregation_capacity_bytes, popularity_collection_change_ratio, propagation_latency_clientedge_us, propagation_latency_crossedge_us, propagation_latency_edgecloud_us, topk_edgecnt)
    {
        assert(cache_name != Util::COVERED_CACHE_NAME);

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }
        
    BasicEdgeWrapper::~BasicEdgeWrapper()
    {
    }

    // (1) Const getters

    uint32_t BasicEdgeWrapper::getTopkEdgecntForPlacement() const
    {
        std::ostringstream oss;
        oss << "should NOT invoke getTopkEdgecntForPlacement() for baselines!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        return 0;
    }

    CoveredCacheManager* BasicEdgeWrapper::getCoveredCacheManagerPtr() const
    {
        std::ostringstream oss;
        oss << "should NOT invoke getCoveredCacheManagerPtr() for baselines!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        return NULL;
    }

    BackgroundCounter& BasicEdgeWrapper::getEdgeBackgroundCounterForBeaconServerRef()
    {
        std::ostringstream oss;
        oss << "should NOT invoke getEdgeBackgroundCounterForBeaconServerRef() for baselines!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
    }

    WeightTuner& BasicEdgeWrapper::getWeightTunerRef()
    {
        std::ostringstream oss;
        oss << "should NOT invoke getWeightTunerRef() for baselines!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
    }

    // (3) Invalidate and unblock for MSI protocol

    MessageBase* BasicEdgeWrapper::getInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;

        // Prepare invalidation request to invalidate the cache copy
        MessageBase* invalidation_request_ptr = new InvalidationRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        assert(invalidation_request_ptr != NULL);

        return;
    }

    void BasicEdgeWrapper::processInvalidationResponse_(MessageBase* invalidation_response_ptr) const
    {
        assert(invalidation_response_ptr != NULL);
        assert(invalidation_response_ptr->getMessageType() == MessageType::kInvalidationResponse);
        return;
    }

    MessageBase* BasicEdgeWrapper::getFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;

        // Prepare finish block request to finish blocking for writes in all closest edge nodes
        MessageBase* finish_block_request_ptr = new FinishBlockRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        assert(finish_block_request_ptr != NULL);

        return;
    }

    void BasicEdgeWrapper::processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const
    {
        assert(finish_block_response_ptr != NULL);
        assert(finish_block_response_ptr->getMessageType() == MessageType::kFinishBlockResponse);
        return;
    }

    // (5) Other utilities

    void BasicEdgeWrapper::checkPointers_() const
    {
        EdgeWrapperBase::checkPointers_();

        return;
    }

    // (6) Common utility functions

    // (6.1) For local edge cache access

    bool BasicEdgeWrapper::getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value) const
    {
        // Cache server gets local edge cache for foreground local data requests
        // Beacon server gets local edge cache for background non-blocking data fetching

        checkPointers_();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_and_valid = edge_cache_ptr_->get(key, is_redirected, value, affect_victim_tracker);

        return is_local_cached_and_valid;
    }

    // (6.2) For local directory admission

    void BasicEdgeWrapper::admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const bool& skip_propagation_latency) const
    {
        // Cache server admits local directory for foreground independent admission
        // Beacon server admits local directory for background non-blocking placement notification at local/remote beacon edge node

        checkPointers_();

        uint32_t current_edge_idx = getNodeIdx();
        const bool is_admit = true; // Admit content directory
        MetadataUpdateRequirement metadata_update_requirement;
        cooperation_wrapper_ptr_->updateDirectoryTable(key, current_edge_idx, is_admit, directory_info, is_being_written, is_neighbor_cached, metadata_update_requirement);

        return;
    }

    // (7) Method-specific functions

    void BasicEdgeWrapper::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << funcname << " is not supported in constCustomFunc()";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }
}