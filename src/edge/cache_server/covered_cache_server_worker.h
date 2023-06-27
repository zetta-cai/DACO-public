/*
 * CoveredCacheServerWorker: cache server worker in edge for COVERED.
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

        // (1) Process data requests
        
        // Return if edge node is finished
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& network_addr) const override;

        // (2) Access cooperative edge cache

        // (2.1) Fetch data from neighbor edge nodes

        // Return if edge node is finished
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override; // Check remote directory info
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const override; // Request redirection

        // (2.2) Update content directory information

        // Return if edge node is finished
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const override; // Update remote directory info

        // (2.3) Process writes and block for MSI protocol

        // Return if edge node is finished
        virtual bool acquireBeaconWritelock_(const Key& key, LockResult& lock_result) override;
        virtual bool releaseBeaconWritelock_(const Key& key) override;

        // (5) Admit uncached objects in local edge cache

        // Return if edge node is finished
        virtual bool triggerIndependentAdmission_(const Key& key, const Value& value) const override;

        // Const variable
        std::string instance_name_;
    };
}

#endif