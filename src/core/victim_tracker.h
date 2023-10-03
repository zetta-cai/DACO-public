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
#include <unordered_set>

#include "common/key.h"
#include "concurrency/rwlock.h"
#include "core/popularity/edgeset.h"
#include "core/victim/edgelevel_victim_metadata.h"
#include "core/victim/victim_dirinfo.h"
#include "core/victim/victim_syncset.h"

namespace covered
{
    class VictimTracker
    {
    public:
        VictimTracker(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt);
        ~VictimTracker();

        // For local synced/beaconed victims
        void updateLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets); // For updates on local cached metadata, which affects cacheinfos and dirinfos of local synced victims
        void updateLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info); // For updates on content directory information, which affects dirinfos of local beaconed victims

        // For victim synchronization
        VictimSyncset getVictimSyncset() const;
        void updateForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets);

        // For trade-off-aware placement calculation
        // NOTE: placement_edgeset is used for preserved edgeset, old local uncached popularities removal; placement_peredge_victimset is used for victim removal; victim_fetch_edgeset is used for lazy victim fetching (all under non-blocking placement deployment)
        DeltaReward calcEvictionCost(const ObjectSize& object_size, const Edgeset& placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& placement_peredge_victimset, Edgeset& victim_fetch_edgeset) const;

        // For non-blocking placement deployment
        void removeVictimsForGivenEdge(const uint32_t& edge_idx, const std::unordered_set<Key, KeyHasher>& victim_keyset); // NOTE: removed victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node

        uint64_t getSizeForCapacity() const;
    private:
        // NOTE: the list of VictimCacheinfos follows the ascending order of local rewards
        typedef std::unordered_map<uint32_t, EdgelevelVictimMetadata> peredge_victim_metadata_t;
        typedef std::unordered_map<Key, VictimDirinfo, KeyHasher> perkey_victim_dirinfo_t;

        static const std::string kClassName;

        // Utils

        void replaceVictimMetadataForEdgeIdx_(const uint32_t& edge_idx, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& synced_victim_cacheinfos); // Replace cache margin bytes and cacheinfos of local/neighbor synced victims for a specific edge node
        void replaceVictimDirinfoSets_(const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& beaconed_synced_victim_dirinfosets, const bool& is_local_beaconed); // Replace dirinfoset in existing VictimDirinfo if any of each local/neighbor beaconed victim
        void tryToReleaseVictimDirinfo_(const Key& key); // Decrease refcnt of existing VictimDirinfo if any for the given key and release space if refcnt becomes zero

        // NOTE: pervictim_edgeset and pervictim_cacheinfos are used for eviction cost in placement calculation, while peredge_victimset is used for victim removal in non-blocking placement deployment
        void findVictimsForPlacement_(const ObjectSize& object_size, const Edgeset& placement_edgeset, std::unordered_map<Key, Edgeset, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_victimset, Edgeset& victim_fetch_edgeset) const; // Find victims from placement edgeset if admit a hot object with the given size
        bool isLastCopiesForVictimEdgeset_(const Key& key, const Edgeset& victim_edgeset) const; // Check whether the victim edgeset is the last cache copies of the given key

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