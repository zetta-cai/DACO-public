/*
 * CoveredCacheServerWorker: cache server worker in edge for COVERED.
 *
 * NOTE: for each local cache miss, we piggyback local uncached popularity if any into existing cooperation control message (e.g., directory lookup request) to update aggregated uncached popularity in beacon node without extra message overhead.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef COVERED_CACHE_SERVER_WORKER_H
#define COVERED_CACHE_SERVER_WORKER_H

#include <string>

#include "edge/cache_server/cache_server_worker_base.h"

namespace covered
{
    class CoveredCacheServerWorker : public CacheServerWorkerBase
    {
    public:
        CoveredCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr);
        virtual ~CoveredCacheServerWorker();
    private:
        static const std::string kClassName;

        // (1.1) Access local edge cache

        // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

        virtual bool lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const override; // Return if edge node is finished

        virtual bool needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        virtual MessageBase* getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint) const override;
        
        virtual MessageBase* getReqToRedirectGet_(const uint32_t& dst_edge_idx_for_compression, const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag) const override;

        // (1.4) Update invalid cached objects in local edge cache

        virtual bool tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is local cached yet invalid

        // (1.5) Trigger cache placement for getrsp (ONLY for COVERED)

        virtual bool tryToTriggerCachePlacementForGetrsp_(const Key& key, const Value& value, const CollectedPopularity& collected_popularity_after_fetch_value, const FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const override; // Return is edge is finished

        // (2.1) Acquire write lock and block for MSI protocol

        virtual bool acquireLocalWritelock_(const Key& key, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) override; // Return if edge node is finished
        virtual MessageBase* getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const override;

        virtual void processReqToFinishBlock_(MessageBase* control_request_ptr) const override;
        virtual MessageBase* getRspToFinishBlock_(MessageBase* control_request_ptr, const BandwidthUsage& tmp_bandwidth_usage) const override;

        // (2.3) Update cached objects in local edge cache

        virtual bool updateLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is cached after udpate
        virtual bool removeLocalEdgeCache_(const Key& key) const override; // Return if key is cached after removal

        // (2.4) Release write lock for MSI protocol

        virtual bool releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency) override; // Return if edge node is finished
        virtual MessageBase* getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual bool processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const override;

        // (3) Process redirected requests (see src/cache_server/cache_server_redirection_processor.*)

        // (4.1) Admit uncached objects in local edge cache

        // (4.2) Admit content directory information

        // Const variable
        std::string instance_name_;
    };
}

#endif