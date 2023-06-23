/*
 * CooperationWrapperBase: the base class to manage metadata for cooperative edge caching with MSI protocol (thread safe).
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from a target edge node by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of the closest edge cache.
 * 
 * NOTE: all non-const shared variables in CooperationWrapperBase should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <string>
#include <unordered_set>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/block_tracker.h"
#include "cooperation/dht_wrapper.h"
#include "cooperation/directory_info.h"
#include "cooperation/directory_table.h"
#include "edge/edge_param.h"

namespace covered
{
    class CooperationWrapperBase
    {
    public:
        static CooperationWrapperBase* getCooperationWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);

        CooperationWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~CooperationWrapperBase();

        // (1) Locate beacon edge node

        uint32_t getBeaconEdgeIdx(const Key& key) const;
        NetworkAddr getBeaconEdgeBeaconServerAddr(const Key& key) const;

        // (2) Access content directory information

        void lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const; // Check local directory information
        void updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written); // Update local directory information

        // (3) Access blocklist

        // Buffer closest edge nodes waiting for writes
        // NOTE: the blocked edge nodes will be notified after writes
        void addEdgeIntoBlocklist(const Key& key, const NetworkAddr& network_addr);
        std::unordered_set<NetworkAddr, NetworkAddrHasher> getBlocklistIfNoWrite(const Key& key);

        // (4) Process writes for MSI protocol

        bool acquireLocalWritelock(const Key& key, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo);

        // (5) Get size for capacity check

        uint32_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string base_instance_name_;
        DhtWrapper* dht_wrapper_ptr_;

        // Non-const shared variables (cooperation metadata)
        DirectoryTable* directory_table_ptr_; // per-key content directory infos (thread safe)
        BlockTracker block_tracker_; // per-key cooperation metadata (thread safe)
    };
}

#endif