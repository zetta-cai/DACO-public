/*
 * BasicEdgeWrapper: a baseline edge node launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper.
 *
 * NOTE: all non-const shared variables in EdgeWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2024.01.29).
 */

#ifndef BASIC_EDGE_WRAPPER_H
#define BASIC_EDGE_WRAPPER_H

#include <string>

namespace covered
{
    // Forward declaration
    class BasicEdgeWrapper;
}

#include "edge/edge_wrapper_base.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    class BasicEdgeWrapper : public EdgeWrapperBase
    {
    public:
        BasicEdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint64_t& local_uncached_capacity_bytes, const uint64_t& local_uncached_lru_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const CLILatencyInfo& cli_latency_info, const uint32_t& topk_edgecnt, const std::string& realnet_option, const std::string& realnet_expname, const std::vector<uint32_t> _p2p_latency_array = std::vector<uint32_t>());
        virtual ~BasicEdgeWrapper();

        // (1) Const getters

        virtual uint32_t getTopkEdgecntForPlacement() const override;
        virtual CoveredCacheManager* getCoveredCacheManagerPtr() const override;
        virtual WeightTuner& getWeightTunerRef() override;

        // (3) Invalidate and unblock for MSI protocol

        // NOTE: NO need to add events of issue_invalidation_req, as they happen in parallel and have been counted in the event of invalidate_cache_copies
        virtual MessageBase* getInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const ExtraCommonMsghdr& extra_common_msghdr) const override;
        virtual void processInvalidationResponse_(MessageBase* invalidation_response_ptr) const override;
        
        // NOTE: NO need to add events of issue_finish_block_req, as they happen in parallel and have been counted in the event of finish_block
        virtual MessageBase* getFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const ExtraCommonMsghdr& extra_common_msghdr) const override;
        virtual void processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const override;

        // (6) Common utility functions (invoked by edge cache server worker/placement-processor or edge beacon server of closest/beacon edge node)

        // (6.1) For local edge cache access
        virtual bool getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value, bool& is_tracked_before_fetch_value) const override; // Return is local cached and valid

        // (6.2) For local directory admission
        virtual void admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const ExtraCommonMsghdr& extra_common_msghdr) const override; // Admit directory info in current edge node (is_neighbor_cached indicates if key is cached by any other edge node except the current edge node after admiting local dirinfo; invoked by cache server worker or beacon server for local placement notification if sender is or not beacon)

        // (7) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const override;

        // (8) Evaluation-related functions
        virtual void dumpEdgeSnapshot_(std::fstream* fs_ptr) const override;
        virtual void loadEdgeSnapshot_(std::fstream* fs_ptr) override;
    private:
        // Perform clustering for MagNet
        void clusterForMagnet_(const Key& key, const std::list<DirectoryInfo>& dirinfo_set, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const;
    private:
        static const std::string kClassName;

        // (5) Other utilities
 
        virtual void checkPointers_() const;

        std::string instance_name_;

        HashWrapperBase* hash_wrapper_for_magnet_ptr_; // Used by MagNet for clustering
    };
}

#endif