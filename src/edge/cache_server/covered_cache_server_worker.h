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

        virtual bool getLocalEdgeCache_(const Key& key, Value& value) const override; // Return is local cached and valid

        // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

        virtual void lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, const bool& skip_propagation_latency) const override;

        virtual bool needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        virtual MessageBase* getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        
        virtual MessageBase* getReqToRedirectGet_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag) const override;

        // (1.4) Update invalid cached objects in local edge cache

        virtual bool tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is local cached yet invalid

        // (2.1) Acquire write lock and block for MSI protocol

        virtual void acquireLocalWritelock_(const Key& key, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo) override;
        virtual MessageBase* getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const override;

        // (2.3) Update cached objects in local edge cache

        virtual bool updateLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is cached after udpate
        virtual bool removeLocalEdgeCache_(const Key& key) const override; // Return if key is cached after removal

        // (2.4) Release write lock for MSI protocol

        virtual void releaseLocalWritelock_(const Key& key, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, const bool& skip_propagation_latency) override;
        virtual MessageBase* getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr) const override;

        // (3) Process redirected requests

        virtual void processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const override;
        virtual MessageBase* getRspForRedirectedGet_(const Key& key, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const override;

        // (4.1) Admit uncached objects in local edge cache

        virtual void admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_valid) const override;

        // (4.2) Evict cached objects from local edge cache

        virtual void evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const override;

        // (4.3) Update content directory information

        virtual void updateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, const bool& skip_propagation_latency) const override; // Update directory info in current edge node

        virtual MessageBase* getReqToUpdateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const override;
        virtual void processRspToUpdateBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written) const override;

        // Const variable
        std::string instance_name_;
    };
}

#endif