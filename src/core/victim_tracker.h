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
#include "cooperation/cooperation_wrapper_base.h"
#include "concurrency/rwlock.h"
#include "core/popularity/edgeset.h"
#include "core/victim/edgelevel_victim_metadata.h"
#include "core/victim/victim_dirinfo.h"
#include "core/victim/victim_syncset.h"
#include "core/victim/victimsync_monitor.h"

namespace covered
{
    class VictimTracker
    {
    public:
        VictimTracker(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt);
        ~VictimTracker();

        // For local synced/beaconed victims
        void updateLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets, const CooperationWrapperBase* cooperation_wrapper_ptr); // For updates on local cached metadata, which affects cacheinfos and dirinfos of local synced victims
        void updateLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info); // For updates on content directory information, which affects dirinfos of local beaconed victims

        // For victim synchronization
        // NOTE: getLocalVictimSyncsetForSynchronization() can ONLY be used to issue local synced victims for victim synchronization, as it will increase cur_seqnum_ of the current edge node towards the given dst edge node in peredge_victimsync_monitor_
        VictimSyncset getLocalVictimSyncsetForSynchronization(const uint32_t& dst_edge_idx_for_compression, const uint64_t& latest_local_cache_margin_bytes) const; // Get complete victim syncset for current edge node (i.e., edge_idx_) (NOTE: we always use the latest cache margin bytes for local victim syncset, instead of using that in edge-level victim metadata of the current edge node, which may be stale)
        void updateForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets, const CooperationWrapperBase* cooperation_wrapper_ptr); // Update victim tracker in the current edge node for the received victim syncset from neighbor edge node (neighbor_victim_syncset may be complete/compressed)

        // For trade-off-aware placement calculation
        // NOTE: placement_edgeset is used for preserved edgeset, old local uncached popularities removal; placement_peredge_synced_victimset is used for synced victim removal from victim tracker, while placement_peredge_fetched_victimset is used for fetched victim removal from victim cache; victim_fetch_edgeset is used for lazy victim fetching (all under non-blocking placement deployment)
        DeltaReward calcEvictionCost(const ObjectSize& object_size, const Edgeset& placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& placement_peredge_synced_victimset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& placement_peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos = std::unordered_map<uint32_t, std::list<VictimCacheinfo>>(), const std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset = std::unordered_map<Key, DirinfoSet, KeyHasher>()) const;

        // For non-blocking placement deployment
        void removeVictimsForGivenEdge(const uint32_t& edge_idx, const std::unordered_set<Key, KeyHasher>& victim_keyset); // NOTE: removed victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node

        uint64_t getSizeForCapacity() const;
    private:
        // NOTE: the list of VictimCacheinfos follows the ascending order of local rewards
        typedef std::unordered_map<uint32_t, EdgelevelVictimMetadata> peredge_victim_metadata_t;
        typedef std::unordered_map<Key, VictimDirinfo, KeyHasher> perkey_victim_dirinfo_t;
        typedef std::unordered_map<uint32_t, VictimSyncset> peredge_victim_syncset_t;
        typedef std::unordered_map<uint32_t, VictimsyncMonitor> peredge_victimsync_monitor_t;
        
        static const std::string kClassName;

        // Utils

        // For victim synchronization
        SeqNum getAndIncrCurSeqnum_(const uint32_t& dst_edge_idx_for_compression) const;
        bool checkAndResetNeedEnforcement_(const uint32_t& dst_edge_idx_for_compression) const;
        VictimSyncset getVictimSyncset_(const uint32_t& edge_idx, const SeqNum& seqnum, const bool& is_enforce_complete) const; // For local edge idx to synchronize victim info, seqnum is the cur_seqnum_ of to-be-issued local victim syncset; for neighbor edge idx to recover complete victim info, seqnum is the tracked_seqnum_ of existing tracked victim info synced from neighbor before
        bool replacePrevVictimSyncset_(const uint32_t& dst_edge_idx_for_compression, const VictimSyncset& current_victim_syncset, VictimSyncset& prev_victim_syncset) const; // Return if prev victim syncset for dst edge idx exists

        // For victim update and removal
        void replaceVictimMetadataForEdgeIdx_(const uint32_t& edge_idx, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& synced_victim_cacheinfos, const CooperationWrapperBase* cooperation_wrapper_ptr); // Replace cache margin bytes and cacheinfos of local/neighbor synced victims for a specific edge node (synced_victim_cacheinfos MUST be complete)
        void replaceVictimDirinfoSets_(const std::unordered_map<Key, DirinfoSet, KeyHasher>& beaconed_synced_victim_dirinfosets, const bool& is_local_beaconed); // Replace dirinfoset in existing VictimDirinfo if any of each local/neighbor beaconed victim
        void tryToReleaseVictimDirinfo_(const Key& key); // Decrease refcnt of existing VictimDirinfo if any for the given key and release space if refcnt becomes zero

        // For trade-off-aware placement calculation
        // NOTE: pervictim_edgeset and pervictim_cacheinfos are used for eviction cost in placement calculation, while peredge_synced_victimset and peredge_fetched_victimset are used for victim removal in non-blocking placement deployment
        void findVictimsForPlacement_(const ObjectSize& object_size, const Edgeset& placement_edgeset, std::unordered_map<Key, Edgeset, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_synced_victimset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos) const; // Find victims from placement edgeset if admit a hot object with the given size
        bool isLastCopiesForVictimEdgeset_(const Key& key, const Edgeset& victim_edgeset, const std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset) const; // Check whether the victim edgeset is the last cache copies of the given key

        void checkPointers_() const;

        // Const shared variables
        const uint32_t edge_idx_;
        std::string instance_name_;
        const uint32_t peredge_synced_victimcnt_; // Come from CLI
        const uint32_t peredge_monitored_victimsetcnt_; // Come from CLI

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as VictimTracker's data structures have two types of keys (edge index and Key)
        mutable Rwlock* rwlock_for_victim_tracker_;

        // Non-const shared varaibles
        mutable uint64_t size_bytes_; // Cache size usage of victim tracker
        peredge_victim_metadata_t peredge_victim_metadata_; // NOTE: even if all victim cacheinfos in the edge-level victim metadata are removed after placement calculation, we will NOT erase the edge-level victim metadata from peredge_victim_metadata_, as we need to find victims based on its cache margin bytes (see VictimTracker::findVictimsForPlacement_())
        perkey_victim_dirinfo_t perkey_victim_dirinfo_;
        mutable peredge_victimsync_monitor_t peredge_victimsync_monitor_; // Sequence-based victim synchronization monitor for each source/dst edge node
    };
}

#endif