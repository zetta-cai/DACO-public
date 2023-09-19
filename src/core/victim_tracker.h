/*
 * VictimTracker: track peredge_synced_victimcnt victims with the least local rewards (w1 * local cached popularity + w2 * redirected cached popularity) for each edge node (thread safe).
 *
 * NOTE: the victims are synced across edge nodes by piggybacking, while extra victims will be lazily fetched by beacon node if synced victims are not enough for admission.
 *
 * NOTE: we call the synced victims from the current edge node as local synced victims, while those from other edge nodes as neighbor synced victims.
 * (1) A part of local/neighbor synced victims are beaconed by the current edge node (i.e., local beaconed victims); others are neighbor beaconed victims.
 * (2) DirectoryInfos of local beaconed victims are updated for replacement of local synced victims triggered by local edge accesses, replacement of neighbor synced victims triggered by received victim syncset, and local/remote directory updates.
 * (3) DirectoryInfos of neighbor beaconed victims are updated for replacement of neighbor synced victims triggered by received victim syncset.
 * 
 * NOTE: we should pass DirectoryInfos of local beaconed keys for each update of VictimTracker.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_TRACKER_H
#define VICTIM_TRACKER_H

#include <list>
#include <string>
#include <unordered_map>

#include "common/key.h"
#include "concurrency/rwlock.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_dirinfo.h"
#include "core/victim/victim_syncset.h"

namespace covered
{
    class EdgeLevelVictimMetadata
    {
    public:
        EdgeLevelVictimMetadata();
        EdgeLevelVictimMetadata(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos);
        ~EdgeLevelVictimMetadata();

        uint64_t getCacheMarginBytes() const;
        std::list<VictimCacheinfo> getVictimCacheinfos() const;

        const EdgeLevelVictimMetadata& operator=(const EdgeLevelVictimMetadata& other);
    private:
        static const std::string kClassName;

        uint64_t cache_margin_bytes_;
        std::list<VictimCacheinfo> victim_cacheinfos_;
    };

    class VictimTracker
    {
    public:
        VictimTracker(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt);
        ~VictimTracker();

        void updateLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets); // For updates on local cached metadata, which affects cacheinfos and dirinfos of local synced victims
        void updateSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info); // For updates on content directory information, which affects dirinfos of local beaconed victims

        // For victim synchronization
        VictimSyncset getVictimSyncset() const;
        void updateForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets);

        uint64_t getSizeForCapacity() const;
    private:
        // NOTE: the list of VictimCacheinfos follows the ascending order of local rewards
        typedef std::unordered_map<uint32_t, EdgeLevelVictimMetadata> peredge_victim_metadata_t;
        typedef std::unordered_map<Key, VictimDirinfo, KeyHasher> perkey_victim_dirinfo_t;

        static const std::string kClassName;

        void replaceVictimMetadataForEdgeIdx_(const uint32_t& edge_idx, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& synced_victim_cacheinfos); // Replace cache margin bytes and cacheinfos of local/neighbor synced victims for a specific edge node
        void replaceVictimDirinfoSets_(const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& beaconed_synced_victim_dirinfosets, const bool& is_local_beaconed); // Replace VictimDirinfo::dirinfoset of each local/neighbor beaconed victim

        void checkPointers_() const;

        // Const shared variables
        const uint32_t edge_idx_;
        std::string instance_name_;
        const uint32_t peredge_synced_victimcnt_; // Come from CLI

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as VictimTracker's data structures have two types of keys (edge index and Key)
        mutable Rwlock* rwlock_for_victim_tracker_;

        // Non-const shared varaibles
        // TODO: Maintain per-edge-node margin bytes to decide whether to perform placement calculation or not
        uint64_t size_bytes_; // Cache size usage of victim tracker
        peredge_victim_metadata_t peredge_victim_metadata_;
        perkey_victim_dirinfo_t perkey_victim_dirinfo_;
    };
}

#endif