/*
 * CooperationWrapperBase: the base class to manage metadata for cooperative edge caching with MSI protocol (thread safe).
 * 
 * NOTE: all non-const shared variables in CooperationWrapperBase should be thread safe.
 * 
 * NOTE: CooperationWrapper is used by beacon edge node to maintain content diretory information and MSI metadata, while DirectoryCacher is used by sender edge node to cache remote valid directory information, so we maintain DirectoryCacher in CoveredCacheMananger instead of CooperationWrapper.
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
#include "cooperation/directory/dirinfo_set.h"
#include "cooperation/directory_table.h"
#include "core/victim/victim_syncset.h"
#include "message/message_base.h"

namespace covered
{
    class CooperationWrapperBase
    {
    public:
        static CooperationWrapperBase* getCooperationWrapperByCacheName(const std::string& cache_name, const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);

        CooperationWrapperBase(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~CooperationWrapperBase();

        // (0) Get dirinfo of local beaconed keys over the given keyset (NOTE: we do NOT guarantee the atomicity for thess keyset-level functions due to per-key fine-grained locking in cooperation wrapper) (ONLY for COVERED)

        virtual void getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_neighbor_synced_victim_dirinfosets) const = 0; // NOTE: all edge cache/beacon/invalidation servers will access cooperation wrapper to get content directory information for local beaconed victims from received victim syncset
        virtual void getLocalBeaconedVictimsFromCacheinfos(const std::list<VictimCacheinfo>& victim_cacheinfos, std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victim_dirinfosets) const = 0;

        // (1) Locate beacon edge node

        uint32_t getBeaconEdgeIdx(const Key& key) const;
        NetworkAddr getBeaconEdgeBeaconServerRecvreqAddr(const Key& key) const;

        // (2) Access content directory table and block tracker for MSI protocol

        void isGlobalAndSourceCached(const Key& key, const uint32_t& source_edge_idx, bool& is_global_cached, bool& is_source_cached) const;
        bool isBeingWritten(const Key& key) const;
        DirinfoSet getLocalDirectoryInfos(const Key& key) const;

        // Return whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        bool lookupDirectoryTableByCacheServer(const Key& key, const uint32_t& source_edge_idx, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, bool& is_source_cached) const; // Check local directory information (NOTE: find a non-source valid directory info if any)
        bool lookupDirectoryTableByBeaconServer(const Key& key, const uint32_t& source_edge_idx, const NetworkAddr& cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, bool& is_source_cached); // Check local directory information (NOTE: find a non-source valid directory info if any)
        bool updateDirectoryTable(const Key& key, const uint32_t& source_edge_idx, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, MetadataUpdateRequirement& metadata_update_requirement); // Update local directory information

        LockResult acquireLocalWritelockByCacheServer(const Key& key, const uint32_t& source_edge_idx, DirinfoSet& all_dirinfo, bool& is_source_cached);
        LockResult acquireLocalWritelockByBeaconServer(const Key& key, const uint32_t& source_edge_idx, const NetworkAddr& cache_server_worker_recvreq_dst_addr, DirinfoSet& all_dirinfo, bool& is_source_cached);
        std::unordered_set<NetworkAddr, NetworkAddrHasher> releaseLocalWritelock(const Key& key, const uint32_t& source_edge_idx, const DirectoryInfo& sender_dirinfo, bool& is_source_cached);

        // (3) Other functions

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string base_instance_name_;
    protected:
        // (2) Access content directory information

        // NOTE: find a non-source valid directory info if any
        bool lookupDirectoryTable_(const Key& key, const uint32_t& source_edge_id, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, bool& is_source_cached) const; // Return whether the key is cached by a local/neighbor edge node (even if invalid temporarily)

        // (3) Other functions

        void checkPointers_() const;

        // Member varaibles

        // Const shared variables
        const uint32_t edge_idx_;
        DhtWrapper* dht_wrapper_ptr_;

        // Fine-graind locking
        mutable PerkeyRwlock* cooperation_wrapper_perkey_rwlock_ptr_;

        // Non-const shared variables (cooperation metadata)
        DirectoryTable* directory_table_ptr_; // per-key content directory infos (thread safe)
        BlockTracker* block_tracker_ptr_; // per-key cooperation metadata (thread safe)
    };
}

#endif