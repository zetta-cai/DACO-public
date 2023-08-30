/*
 * CooperationWrapperBase: the base class to manage metadata for cooperative edge caching with MSI protocol (thread safe).
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
#include "concurrency/perkey_rwlock.h"
#include "cooperation/block_tracker.h"
#include "cooperation/dht_wrapper.h"
#include "cooperation/directory/directory_info.h"
#include "cooperation/directory_table.h"
#include "message/message_base.h"

namespace covered
{
    class CooperationWrapperBase
    {
    public:
        static CooperationWrapperBase* getCooperationWrapperByCacheName(const std::string& cache_name, const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);

        CooperationWrapperBase(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~CooperationWrapperBase();

        // (1) Locate beacon edge node

        uint32_t getBeaconEdgeIdx(const Key& key) const;
        NetworkAddr getBeaconEdgeBeaconServerRecvreqAddr(const Key& key) const;

        // (2) Access content directory table and block tracker for MSI protocol

        void lookupLocalDirectoryByCacheServer(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const; // Check local directory information
        void lookupLocalDirectoryByBeaconServer(const Key& key, const NetworkAddr& cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info); // Check local directory information
        bool updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info); // Update local directory information; return if key is being written

        LockResult acquireLocalWritelockByCacheServer(const Key& key, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo);
        LockResult acquireLocalWritelockByBeaconServer(const Key& key, const NetworkAddr& cache_server_worker_recvreq_dst_addr, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo);
        std::unordered_set<NetworkAddr, NetworkAddrHasher> releaseLocalWritelock(const Key& key, const DirectoryInfo& sender_dirinfo);

        // (3) Other functions

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // (2) Access content directory information

        void lookupLocalDirectory_(const Key& key, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const;

        // (3) Other functions

        void checkPointers_() const;

        // Member varaibles

        // Const shared variables
        std::string base_instance_name_;
        DhtWrapper* dht_wrapper_ptr_;

        // Fine-graind locking
        mutable PerkeyRwlock* cooperation_wrapper_perkey_rwlock_ptr_;

        // Non-const shared variables (cooperation metadata)
        DirectoryTable* directory_table_ptr_; // per-key content directory infos (thread safe)
        BlockTracker* block_tracker_ptr_; // per-key cooperation metadata (thread safe)
    };
}

#endif