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

        virtual void lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;

        virtual MessageBase* getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        
        virtual bool redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, EventList& event_list, const bool& skip_propagation_latency) const override; // Request redirection

        // (1.4) Update invalid cached objects in local edge cache

        virtual bool tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is local cached yet invalid

        // (2.1) Acquire write lock and block for MSI protocol

        // Return if edge node is finished
        virtual bool acquireBeaconWritelock_(const Key& key, LockResult& lock_result, EventList& event_list, const bool& skip_propagation_latency) override;

        // (2.3) Update cached objects in local edge cache

        virtual bool updateLocalEdgeCache_(const Key& key, const Value& value) const override; // Return if key is cached after udpate
        virtual bool removeLocalEdgeCache_(const Key& key) const override; // Return if key is cached after removal

        // (2.4) Release write lock for MSI protocol

        // Return if edge node is finished
        virtual bool releaseBeaconWritelock_(const Key& key, EventList& event_list, const bool& skip_propagation_latency) override;

        // (3) Process redirected requests

        // (4.1) Admit uncached objects in local edge cache

        // Return if edge node is finished
        virtual bool tryToTriggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list, const bool& skip_propagation_latency) const override;

        // (4.3) Update content directory information

        virtual void updateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const override; // Update directory info in current edge node

        virtual MessageBase* getReqToUpdateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const override;

        // (6) covered-specific utility functions
        
        // For victim synchronization
        void updateCacheManagerForLocalSyncedVictims_() const; // NOTE: ONLY edge cache server worker will access local edge cache, which affects local cached metadata and may trigger update for local synced victims

        // Const variable
        std::string instance_name_;
    };
}

#endif