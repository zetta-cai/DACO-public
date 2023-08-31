/*
 * BasicCacheServerWorker: basic cache server worker in edge for baselines.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef BASIC_CACHE_SERVER_WORKER_H
#define BASIC_CACHE_SERVER_WORKER_H

#include <string>

#include "edge/cache_server/cache_server_worker_base.h"

namespace covered
{
    class BasicCacheServerWorker : public CacheServerWorkerBase
    {
    public:
        BasicCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr);
        virtual ~BasicCacheServerWorker();
    private:
        static const std::string kClassName;

        // (1.1) Access local edge cache

        virtual bool getLocalEdgeCache_(const Key& key, Value& value) const override; // Return is local cached and valid

        // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

        // Return if edge node is finished
        virtual MessageBase* getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const override;
        virtual void processRspToLookupBeaconDirectory_(const DynamicArray& control_response_msg_payload, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, EventList& event_list) const override;
        
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

        // Return if edge node is finished
        virtual void updateCurrentDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const override; // Update directory info in current edge node
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, EventList& event_list, const bool& skip_propagation_latency) const override; // Update directory info in remote edge node

        // Const variable
        std::string instance_name_;
    };
}

#endif