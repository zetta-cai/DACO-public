#include "core/victim_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/kv_list_helper_impl.h"
#include "common/util.h"

namespace covered
{
    // VictimTracker

    const std::string VictimTracker::kClassName = "VictimTracker";

    VictimTracker::VictimTracker(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt) : edge_idx_(edge_idx), peredge_synced_victimcnt_(peredge_synced_victimcnt), peredge_monitored_victimsetcnt_(peredge_monitored_victimsetcnt)
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_victim_tracker_";
        rwlock_for_victim_tracker_ = new Rwlock(oss.str());
        assert(rwlock_for_victim_tracker_ != NULL);

        size_bytes_ = 0;
        peredge_victim_metadata_.resize(edgecnt, EdgelevelVictimMetadata());
        perkey_victim_dirinfo_.clear();
        peredge_victimsync_monitor_.resize(edgecnt, VictimsyncMonitor());
    }

    VictimTracker::~VictimTracker()
    {
        peredge_victim_metadata_.clear();
        peredge_victimsync_monitor_.clear();

        assert(rwlock_for_victim_tracker_ != NULL);
        delete rwlock_for_victim_tracker_;
        rwlock_for_victim_tracker_ = NULL;
    }

    // For local synced/beaconed victims

    void VictimTracker::updateLocalCacheMarginBytes(const uint64_t& local_cache_margin_bytes)
    {
        checkPointers_();

        // Acquire a write lock to update local cache margin bytes atomically
        std::string context_name = "VictimTracker::updateLocalCacheMarginBytes()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // Update local cache margin bytes in edge-level victim metadata
        const bool is_valid_victim_metadata = peredge_victim_metadata_[edge_idx_].isValid();
        if (!is_valid_victim_metadata) // NO edge-level victim metadata for current edge node
        {
            // NOTE: although ONLY update local cache margin bytes means NOT affect local synced victims, they may still NOT be tracked by victim tracker (e.g., the first cache miss for an empty cache)

            // Create a new edge-level victim metadata w/o victim cacheinfos
            peredge_victim_metadata_[edge_idx_] = EdgelevelVictimMetadata(local_cache_margin_bytes, std::list<VictimCacheinfo>());
            assert(peredge_victim_metadata_[edge_idx_].isValid());

            //size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint32_t)); // For edge_idx_
            size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint64_t)); // For local_cache_margin_bytes
        }
        else // Edge-level victim metadata already exists
        {
            peredge_victim_metadata_[edge_idx_].updateCacheMarginBytes(local_cache_margin_bytes);

            // NOTE: NO need to update size_bytes_
        }

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    void VictimTracker::updateLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const CooperationWrapperBase* cooperation_wrapper_ptr)
    {
        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        checkPointers_();

        // Acquire a write lock to update local synced victims atomically
        std::string context_name = "VictimTracker::updateLocalSyncedVictims()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // Replace VictimCacheinfors for local synced victims of current edge node
        // NOTE: local_synced_victim_cacheinfos from local edge cache MUST be complete which has been verified by CoveredLocalCache; local_beaconed_local_synced_victim_dirinfosets from local directory table also MUST be complete which has been verified by EdgeWrapper
        replaceVictimMetadataForEdgeIdx_(edge_idx_, local_cache_margin_bytes, local_synced_victim_cacheinfos, cooperation_wrapper_ptr);

        // Replace VictimDirinfo::dirinfoset for each local beaconed local synced victim of current edge node
        const std::list<std::pair<Key, DirinfoSet>> local_beaconed_local_synced_victim_dirinfosets = cooperation_wrapper_ptr->getLocalBeaconedVictimsFromCacheinfos(local_synced_victim_cacheinfos); // Get directory info sets for local synced victimed beaconed by the current edge node (NOTE: dirinfo sets from local directory table MUST be complete)
        assert(local_beaconed_local_synced_victim_dirinfosets.size() <= local_synced_victim_cacheinfos.size());
        replaceVictimDirinfoSets_(local_beaconed_local_synced_victim_dirinfosets, true);

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    void VictimTracker::updateLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        checkPointers_();

        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "VictimTracker::updateLocalBeaconedVictimDirinfo()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // Update directory info if the local beaconed key is a local/neighbor synced victim
        perkey_victim_dirinfo_t::iterator dirinfo_list_iter = KVListHelper<Key, VictimDirinfo>::findVFromListForK(key, perkey_victim_dirinfo_);
        if (dirinfo_list_iter != perkey_victim_dirinfo_.end()) // The beaconed key is a local/neighbor synced victim
        {
            //assert(dirinfo_list_iter->second.isLocalBeaconed());
            assert(dirinfo_list_iter->second.getBeaconEdgeIdx() == edge_idx_);
            
            // Update directory info for the key
            bool is_update = dirinfo_list_iter->second.updateDirinfoSet(is_admit, directory_info);

            if (is_update) // Update size_bytes_ only if directory info is added/removed successfully
            {
                if (is_admit)
                {
                    size_bytes_ = Util::uint64Add(size_bytes_, directory_info.getSizeForCapacity()); // For added directory info
                }
                else
                {
                    size_bytes_ = Util::uint64Minus(size_bytes_, directory_info.getSizeForCapacity()); // For removed directory info
                }
            }
        }

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    // For victim synchronization

    //VictimSyncset VictimTracker::getLocalVictimSyncsetForSynchronization(const uint32_t& dst_edge_idx_for_compression, const uint64_t& latest_local_cache_margin_bytes) const
    VictimSyncset VictimTracker::getLocalVictimSyncsetForSynchronization(const uint32_t& dst_edge_idx_for_compression, const uint64_t& latest_local_cache_margin_bytes, const Key& key) const // TMPDWEBUG231211
    {
        // TMPDEBUG231211
        struct timespec t0 = Util::getCurrentTimespec();

        checkPointers_();

        // Acquire a write lock to get local victim syncset atomically (NOTE: we need write lock here as we need to update VictimsyncMonitor)
        std::string context_name = "VictimTracker::getLocalVictimSyncsetForSynchronization()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // TMPDEBUG231211
        struct timespec t1 = Util::getCurrentTimespec();

        // Get local complete victim syncset of the current edge node
        const SeqNum cur_seqnum = getAndIncrCurSeqnum_(dst_edge_idx_for_compression); // NOTE: this will increase cur_seqnum_ for the given dst edge node
        const bool need_enforcement = checkAndResetNeedEnforcement_(dst_edge_idx_for_compression); // NOTE: this will reset need_enforcement_ for the given dst edge node if need enforcement
        VictimSyncset current_victim_syncset = getVictimSyncset_(edge_idx_, cur_seqnum, need_enforcement); // NOTE: set VictimSyncset::is_enforce_complete as true if need enforcement (dst edge node will enforce complete victim syncset for the next message towards the current edge node)
        assert(current_victim_syncset.isComplete());

        // TMPDEBUG231211
        struct timespec t2 = Util::getCurrentTimespec();

        // NOTE: we always use the latest cache margin bytes for local victim syncset, instead of using that in edge-level victim metadata of the current edge node, which may be stale
        current_victim_syncset.setCacheMarginBytes(latest_local_cache_margin_bytes);
        assert(current_victim_syncset.isComplete());

        // Replace previously-issued complete victim syncset for dst edge idx by current complete victim syncset if necessary
        VictimSyncset prev_victim_syncset;
        bool is_prev_victim_syncset_exist = replacePrevVictimSyncset_(dst_edge_idx_for_compression, current_victim_syncset, prev_victim_syncset);

        rwlock_for_victim_tracker_->unlock(context_name);

        // TMPDEBUG231211
        struct timespec t3 = Util::getCurrentTimespec();

        if (!is_prev_victim_syncset_exist) // No previous victim syncset for dedup/delta-compression
        {
            // TMPDEBUG231211
            struct timespec t4 = Util::getCurrentTimespec();
            Util::dumpVariablesForDebug(instance_name_, 10, "getLocalVictimSyncsetForSynchronization for key", key.getKeystr().c_str(), "t1-t0:", std::to_string(Util::getDeltaTimeUs(t1, t0)).c_str(), "t2-t1:", std::to_string(Util::getDeltaTimeUs(t2, t1)).c_str(), "t3-t2:", std::to_string(Util::getDeltaTimeUs(t3, t2)).c_str(), "t4-t3:", std::to_string(Util::getDeltaTimeUs(t4, t3)).c_str());

            // NOTE: cur_seqnum may NOT be zero here, as prev issued victim syncset may be cleared by complete enforcement notification from dst edge node
            return current_victim_syncset;
        }
        else
        {
            assert(prev_victim_syncset.isComplete());
            assert(cur_seqnum == Util::uint64Add(prev_victim_syncset.getSeqnum(), 1)); // NOTE: dedup-/delta-based victim syncset compression MUST follow strict seqnum order

            // Calculate compressed victim syncset by dedup/delta-compression based on current and prev complete victim syncset
            VictimSyncset compressed_victim_syncset = VictimSyncset::compress(current_victim_syncset, prev_victim_syncset);

            // TMPDEBUG231211
            struct timespec t4 = Util::getCurrentTimespec();
            Util::dumpVariablesForDebug(instance_name_, 10, "getLocalVictimSyncsetForSynchronization for key", key.getKeystr().c_str(), "t1-t0:", std::to_string(Util::getDeltaTimeUs(t1, t0)).c_str(), "t2-t1:", std::to_string(Util::getDeltaTimeUs(t2, t1)).c_str(), "t3-t2:", std::to_string(Util::getDeltaTimeUs(t3, t2)).c_str(), "t4-t3:", std::to_string(Util::getDeltaTimeUs(t4, t3)).c_str());

            return compressed_victim_syncset;
        }
    }

    //void VictimTracker::updateForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset, const CooperationWrapperBase* cooperation_wrapper_ptr)
    void VictimTracker::updateForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset, const CooperationWrapperBase* cooperation_wrapper_ptr, const Key& key) // TMPDEBUG231211
    {
        // NOTE: limited computation overhead to update neighbor synced victim infos, as we track limited number of neighbor synced victims for the specific edge node

        checkPointers_();

        // Acquire a write lock to update local synced victims atomically
        std::string context_name = "VictimTracker::updateForNeighborVictimSyncset()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // Enforce complete victim syncset for the next message to source edge node if necessary
        const bool is_valid_victimsync_monitor = peredge_victimsync_monitor_[source_edge_idx].isValid();
        if (neighbor_victim_syncset.isEnforceComplete()) // Enforce the current edge node to issue complete victim syncset without compression for the given source edge node
        {
            assert(is_valid_victimsync_monitor); // NOTE: if the source edge node notifies the current edge node to enforce complete victim syncset without compression, it means that the current edge node has sent at least one victim syncset to the source edge node -> current edge node MUST have VictimsyncMonitor for the source edge node

            peredge_victimsync_monitor_[source_edge_idx].enforceComplete(); // Enforce the next message to piggyback a complete victim syncset
        }

        // Allocate VictimsyncMonitor for the source edge node if this is the first victim syncset received from the source edge node
        if (!is_valid_victimsync_monitor) // No VictimsyncMonitor means that this is the first victim syncset from the source edge idx
        {
            // NOTE: cur_seqnum_ will be initialized as 0 such that the first victim syncset from current to source will have a seqnum of 0
            // NOTE: is_first_received_ will be initialized as true such that the first received victim syncset can directly update victim tracker
            peredge_victimsync_monitor_[source_edge_idx].validate();
        }
        assert(peredge_victimsync_monitor_[source_edge_idx].isValid());

        // Start from received victim syncset from neighbor
        VictimSyncset neighbor_complete_victim_syncset = neighbor_victim_syncset;
        const SeqNum synced_seqnum = neighbor_victim_syncset.getSeqnum(); // synced_seqnum is actually cur_seqnum of source/neighbor edge idx when syncing the victim syncset to the current edge node
        const bool is_neighbor_victim_syncset_complete = neighbor_victim_syncset.isComplete();

        // Check if this is the first victim syncset received from the source edge node
        bool need_update_victim_tracker_ = false;
        bool is_first_complete_received = peredge_victimsync_monitor_[source_edge_idx].isFirstCompleteReceived();
        if (is_first_complete_received) // This is the first complete victim syncset received from the source edge idx
        {
            if (is_neighbor_victim_syncset_complete) // Complete victim syncset
            {
                assert(synced_seqnum >= 0); // NOTE: seqnum of the first complete victim syncset received from the specific source edge node could >= 0 due to some in-advance complete victim syncset under packet loss/reordering

                peredge_victimsync_monitor_[source_edge_idx].clearFirstCompleteReceived(); // This will set is_first_complete_received_ = false in VictimsyncMonitor

                neighbor_complete_victim_syncset = peredge_victimsync_monitor_[source_edge_idx].tryToClearEnforcementStatus_(neighbor_complete_victim_syncset, synced_seqnum, peredge_monitored_victimsetcnt_); // This will set tracked_seqnum as synced_seqnum, clear stale and continuous cached victim syncsets, and clear enforcement status (set need_enforcement_ = false, enforcement_seqnum_ = 0, and wait_for_complete_victim_syncset_ = false) if necessary in VictimsyncMonitor

                need_update_victim_tracker_ = true; // Directly update victim tracker without recovery
            }
            else // Compressed victim syncset
            {
                need_update_victim_tracker_ = false; // No need to update victim tracker

                peredge_victimsync_monitor_[source_edge_idx].tryToEnableEnforcementStatus_(neighbor_victim_syncset, synced_seqnum, peredge_monitored_victimsetcnt_); // Trigger complete enforcement if cached victim syncsets is full (this will cache compressed victim syncset if cached_victim_syncsets_ is not full, or enable enforcement status (set need_enforcement_ = true, enforcement_seqnum_ = synced_seqnum, and wait_for_complete_victim_syncset_ = true) otherwise)
            }
        }
        else // Decide whether and how to update victim tracker based on existing complete victim syncset tracked for the source edge idx in the current edge node
        {
            // Get tracked seqnum from existing VictimsyncMonitor
            const SeqNum tracked_seqnum = peredge_victimsync_monitor_[source_edge_idx].getTrackedSeqnum();

            // Compare synced seqnum with tracked seqnum
            if (synced_seqnum <= tracked_seqnum) // An outdated victim syncset from source edge node
            {
                need_update_victim_tracker_ = false; // No need to update victim tracker
            }
            else if (synced_seqnum == Util::uint64Add(tracked_seqnum, 1)) // A matched victim syncset
            {
                // Recover neighbor complete victim syncset first if necessary
                if (!is_neighbor_victim_syncset_complete) // If neighbor_victim_syncset is compressed
                {
                    // Get existing complete victim syncset from victim tracker for source edge idx
                    // NOTE: get tracked victim syncset previously-issued by the source edge node for recovery (tracked_seqnum and is_enforce_complete are NOT used during recovery)
                    const bool unused_is_enforce_complete = false;
                    VictimSyncset existing_complete_victim_syncset = getVictimSyncset_(source_edge_idx, tracked_seqnum, unused_is_enforce_complete);
                    assert(existing_complete_victim_syncset.isComplete());

                    // Recover neighbor complete victim syncset based on existing complete victim syncset of source edge idx and received neighbor_victim_syncset if compressed
                    //neighbor_complete_victim_syncset = VictimSyncset::recover(neighbor_victim_syncset, existing_complete_victim_syncset);
                    neighbor_complete_victim_syncset = VictimSyncset::recover(neighbor_victim_syncset, existing_complete_victim_syncset, key); // TMPDEBUG231211
                    assert(neighbor_complete_victim_syncset.isComplete());
                }

                need_update_victim_tracker_ = true; // Update victim tracker with recovered/synced complete vicitm syncset

                neighbor_complete_victim_syncset = peredge_victimsync_monitor_[source_edge_idx].tryToClearEnforcementStatus_(neighbor_complete_victim_syncset, synced_seqnum, peredge_monitored_victimsetcnt_); // This will set tracked_seqnum as synced_seqnum (i.e., tracked_seqnum + 1), clear stale and continuous cached victim syncsets, and clear enforcement status (set need_enforcement_ = false, enforcement_seqnum_ = 0, and wait_for_complete_victim_syncset_ = false) if necessary in VictimsyncMonitor
            }
            else if (is_neighbor_victim_syncset_complete) // A complete victim syncset w/ synced_seqnum > tracked_seqnum + 1
            {
                need_update_victim_tracker_ = true; // Directly use complete victim syncset to update victim tracker

                neighbor_complete_victim_syncset = peredge_victimsync_monitor_[source_edge_idx].tryToClearEnforcementStatus_(neighbor_complete_victim_syncset, synced_seqnum, peredge_monitored_victimsetcnt_); // This will set tracked_seqnum as synced_seqnum (i.e., tracked_seqnum + 1), clear stale and continuous cached victim syncsets, and clear enforcement status (set need_enforcement_ = false, enforcement_seqnum_ = 0, and wait_for_complete_victim_syncset_ = false) if necessary in VictimsyncMonitor
            }
            else // A compressed victim syncset w/ synced_seqnum > tracked_seqnum + 1
            {
                need_update_victim_tracker_ = false; // No need to update victim tracker

                peredge_victimsync_monitor_[source_edge_idx].tryToEnableEnforcementStatus_(neighbor_victim_syncset, synced_seqnum, peredge_monitored_victimsetcnt_); // Trigger complete enforcement if cached victim syncsets is full (this will cache compressed victim syncset if cached_victim_syncsets_ is not full, or enable enforcement status (set need_enforcement_ = true, enforcement_seqnum_ = synced_seqnum, and wait_for_complete_victim_syncset_ = true) otherwise)
            }
        }

        // Update edge-level victim metadata and victim dirinfo sets in victim tracker
        if (need_update_victim_tracker_)
        {
            assert(neighbor_complete_victim_syncset.isComplete()); // NOTE: victim cacheinfos and dirinfo sets of neighbor victim syncset passed into victim tracker MUST be complete

            assert(synced_seqnum <= neighbor_complete_victim_syncset.getSeqnum()); // Complete seqnum should be the same as synced seqnum

            // NOTE: neighbor complete victim syncset MUST be complete
            // Get cache margin bytes
            uint64_t neighbor_cache_margin_bytes = 0;
            int neighbor_cache_margin_delta_bytes = 0;
            bool with_complete_vicitm_syncset = neighbor_complete_victim_syncset.getCacheMarginBytesOrDelta(neighbor_cache_margin_bytes, neighbor_cache_margin_delta_bytes);
            assert(with_complete_vicitm_syncset);
            UNUSED(neighbor_cache_margin_delta_bytes);
            // Get neighbor synced victim cacheinfos
            std::list<VictimCacheinfo> neighbor_synced_victim_cacheinfos;
            with_complete_vicitm_syncset = neighbor_complete_victim_syncset.getLocalSyncedVictims(neighbor_synced_victim_cacheinfos);
            assert(with_complete_vicitm_syncset);
            // Get neighbor beaconed victim dirinfosets
            std::list<std::pair<Key, DirinfoSet>> neighbor_beaconed_victim_dirinfosets;
            with_complete_vicitm_syncset = neighbor_complete_victim_syncset.getLocalBeaconedVictims(neighbor_beaconed_victim_dirinfosets);
            assert(with_complete_vicitm_syncset);

            // Replace VictimCacheinfos for neighbor synced victims of the given edge node
            replaceVictimMetadataForEdgeIdx_(source_edge_idx, neighbor_cache_margin_bytes, neighbor_synced_victim_cacheinfos, cooperation_wrapper_ptr);

            // Try to replace VictimDirinfo::dirinfoset (if any) for each neighbor beaconed neighbor synced victim of the given edge node
            replaceVictimDirinfoSets_(neighbor_beaconed_victim_dirinfosets, false);

            // Replace VictimDirinfo::dirinfoset for each local beaconed neighbor synced victim of the given edge node
            // NOTE: local_beaconed_neighbor_synced_victim_dirinfosets MUST be complete due to from local directory table
            const std::list<std::pair<Key, DirinfoSet>> local_beaconed_neighbor_synced_victim_dirinfosets = cooperation_wrapper_ptr->getLocalBeaconedVictimsFromCacheinfos(neighbor_synced_victim_cacheinfos);
            assert(local_beaconed_neighbor_synced_victim_dirinfosets.size() <= neighbor_synced_victim_cacheinfos.size());
            replaceVictimDirinfoSets_(local_beaconed_neighbor_synced_victim_dirinfosets, true);
        }

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    // For trade-off-aware placement calculation
    
    DeltaReward VictimTracker::calcEvictionCost(const EdgeWrapper* edge_wrapper_ptr, const ObjectSize& object_size, const Edgeset& placement_edgeset, std::list<std::pair<uint32_t, std::list<Key>>>& placement_peredge_synced_victimset, std::list<std::pair<uint32_t, std::list<Key>>>& placement_peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::list<std::pair<uint32_t, std::list<VictimCacheinfo>>>& extra_peredge_victim_cacheinfos, const std::list<std::pair<Key, DirinfoSet>>& extra_perkey_victim_dirinfoset) const
    {
        checkPointers_();

        const bool with_extra_victims = (extra_peredge_victim_cacheinfos.size() > 0);
        DeltaReward eviction_cost = 0.0;
        
        // Acquire a read lock to calculate eviction cost atomically
        std::string context_name = "VictimTracker::calcEvictionCost()";
        rwlock_for_victim_tracker_->acquire_lock_shared(context_name);

        // Find victims from placement edgeset if admit a hot object with the given size (set victim_fetch_edgeset for lazy victim fetching)
        std::list<std::pair<Key, Edgeset>> pervictim_edgeset;
        std::list<std::pair<Key, std::list<VictimCacheinfo>>> pervictim_cacheinfos;
        findVictimsForPlacement_(object_size, placement_edgeset, pervictim_edgeset, pervictim_cacheinfos, placement_peredge_synced_victimset, placement_peredge_fetched_victimset, victim_fetch_edgeset, extra_peredge_victim_cacheinfos);
        if (with_extra_victims)
        {
            assert(victim_fetch_edgeset.size() == 0); // NOTE: we DISABLE recursive victim fetching
        }

        // Enumerate found victims to calculate eviction cost
        for (std::list<std::pair<Key, Edgeset>>::const_iterator pervictim_edgeset_const_iter = pervictim_edgeset.begin(); pervictim_edgeset_const_iter != pervictim_edgeset.end(); pervictim_edgeset_const_iter++)
        {
            // Check if the victim edgeset is the last copies of the given key
            const Key& tmp_victim_key = pervictim_edgeset_const_iter->first;
            const Edgeset& tmp_victim_edgeset_ref = pervictim_edgeset_const_iter->second;
            bool is_last_copies = isLastCopiesForVictimEdgeset_(tmp_victim_key, tmp_victim_edgeset_ref, extra_perkey_victim_dirinfoset);

            // NOTE: we add each pair of edgeidx and cacheinfo of a victim simultaneously in findVictimsForPlacement_() -> tmp_victim_cacheinfos MUST exist and has the same size as tmp_victim_edgeset_ref
            std::list<std::pair<Key, std::list<VictimCacheinfo>>>::const_iterator pervictim_cacheinfos_const_iter = KVListHelper<Key, std::list<VictimCacheinfo>>::findVFromListForK(tmp_victim_key, pervictim_cacheinfos);
            assert(pervictim_cacheinfos_const_iter != pervictim_cacheinfos.end());
            const std::list<VictimCacheinfo>& tmp_victim_cacheinfos = pervictim_cacheinfos_const_iter->second;
            assert(tmp_victim_cacheinfos.size() == tmp_victim_edgeset_ref.size());

            // Calculate eviction cost based on is_last_copies and tmp_victim_cacheinfos
            for (std::list<VictimCacheinfo>::const_iterator victim_cacheinfo_list_const_iter = tmp_victim_cacheinfos.begin(); victim_cacheinfo_list_const_iter != tmp_victim_cacheinfos.end(); victim_cacheinfo_list_const_iter++)
            {
                assert(victim_cacheinfo_list_const_iter->isComplete()); // NOTE: victim cacheinfo from edge-level victim metadata of victim tracker MUST be complete

                // Get local cached and redirected cache popularity of the given victim at the given edge node
                Popularity tmp_local_cached_popularity = 0.0;
                Popularity tmp_redirected_cached_popularity = 0.0;
                bool with_complete_local_cached_popularity = victim_cacheinfo_list_const_iter->getLocalCachedPopularity(tmp_local_cached_popularity);
                assert(with_complete_local_cached_popularity); // NOTE: victim cacheinfo of pervictim_cacheinfos (from peredge_victim_metadata_ in victim tracker) MUST be complete
                bool with_complete_redirected_cached_popularity = victim_cacheinfo_list_const_iter->getRedirectedCachedPopularity(tmp_redirected_cached_popularity);
                assert(with_complete_redirected_cached_popularity); // NOTE: victim cacheinfo of pervictim_cacheinfos (from peredge_victim_metadata_ in victim tracker) MUST be complete

                DeltaReward tmp_eviction_cost = edge_wrapper_ptr->calcLocalCachedReward(tmp_local_cached_popularity, tmp_redirected_cached_popularity, is_last_copies);
                eviction_cost += tmp_eviction_cost;
            }
        }

        rwlock_for_victim_tracker_->unlock_shared(context_name);
        return eviction_cost;
    }

    // For non-blocking placement deployment

    void VictimTracker::removeVictimsForGivenEdge(const uint32_t& edge_idx, const std::list<Key>& victim_keyset)
    {
        checkPointers_();

        // Acquire a write lock to remove victims atomically
        std::string context_name = "VictimTracker::removeVictimsForGivenEdge()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // NOTE: each edge node in placement edgeset MUST have EdgeLevelVictimMetadata
        EdgelevelVictimMetadata& edge_level_victim_metadata_ref = peredge_victim_metadata_[edge_idx];
        assert(edge_level_victim_metadata_ref.isValid());

        // Remove corresponding victim cacheinfos from peredge_victim_metadata_iter->second if any
        uint64_t removed_cacheinfos_size = 0;
        bool is_empty = edge_level_victim_metadata_ref.removeVictimsForPlacement(victim_keyset, removed_cacheinfos_size);
        if (removed_cacheinfos_size > 0)
        {
            size_bytes_ = Util::uint64Minus(size_bytes_, removed_cacheinfos_size); // For removed victim cacheinfos
        }

        // NOTE: even if all victim cacheinfos in the edge-level victim metadata are removed after placement calculation, we will NOT erase the edge-level victim metadata from peredge_victim_metadata_, as we need to find victims based on its cache margin bytes (see VictimTracker::findVictimsForPlacement_())
        /*// Remove empty edge_level_metadata from peredge_victim_metadata_
        if (is_empty)
        {
            peredge_victim_metadata_[edge_idx] = EdgeLevelVictimMetadata();
            assert(!peredge_victim_metadata_[edge_idx].isValid());

            //size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(uint32_t)); // For edge_idx
            size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(uint64_t)); // For cache_margin_bytes
        }*/
        UNUSED(is_empty);

        // Remove corresponding victim dirinfos from perkey_victim_dirinfo_ if refcnt becomes zero
        for (std::list<Key>::const_iterator victim_keyset_const_iter = victim_keyset.begin(); victim_keyset_const_iter != victim_keyset.end(); victim_keyset_const_iter++)
        {
            const Key& tmp_key = *victim_keyset_const_iter;
            tryToReleaseVictimDirinfo_(tmp_key);
        }

        rwlock_for_victim_tracker_->unlock(context_name);
        return;
    }

    // For fast-path single-placement calculation in current edge node (NOT as a beacon node)
    
    DeltaReward VictimTracker::calcEvictionCostForFastPathPlacement(const EdgeWrapper* edge_wrapper_ptr, const std::list<VictimCacheinfo>& curedge_local_cached_victim_cacheinfos, const std::list<std::pair<Key, DirinfoSet>>& curedge_local_beaconed_local_cached_victim_dirinfosets) const
    {
        checkPointers_();

        DeltaReward local_eviction_cost = 0.0;

        // Acquire a read lock to calculate eviction cost atomically
        std::string context_name = "VictimTracker::calcEvictionCostForFastPathPlacement()";
        rwlock_for_victim_tracker_->acquire_lock_shared(context_name);

        // Prepare victim edgeset (ONLY consider a single placement of current edge node for fast path)
        Edgeset tmp_victim_edgeset;
        tmp_victim_edgeset.insert(edge_idx_);

        // Enumerate passed victims found from local edge cache to calculate eviction cost
        for (std::list<VictimCacheinfo>::const_iterator tmp_victim_const_iter = curedge_local_cached_victim_cacheinfos.begin(); tmp_victim_const_iter != curedge_local_cached_victim_cacheinfos.end(); tmp_victim_const_iter++)
        {
            // Check if current edge node is the last copy of the given victim key
            const Key& tmp_victim_key = tmp_victim_const_iter->getKey();
            bool is_last_copies = isLastCopiesForVictimEdgeset_(tmp_victim_key, tmp_victim_edgeset, curedge_local_beaconed_local_cached_victim_dirinfosets);

            // Get local cached and redirected cached popularity of the given victim at the current edge node
            assert(tmp_victim_const_iter->isComplete()); // NOTE: the victim cacheinfo from local edge cache of current edge node MUST be complete
            Popularity tmp_local_cached_popularity = 0.0;
            Popularity tmp_redirected_cached_popularity = 0.0;
            bool with_complete_local_cached_popularity = tmp_victim_const_iter->getLocalCachedPopularity(tmp_local_cached_popularity);
            assert(with_complete_local_cached_popularity); // NOTE: the victim cacheinfo from local edge cache of current edge node MUST be complete
            bool with_complete_redirected_cached_popularity = tmp_victim_const_iter->getRedirectedCachedPopularity(tmp_redirected_cached_popularity);
            assert(with_complete_redirected_cached_popularity); // NOTE: the victim cacheinfo from local edge cache of current edge node MUST be complete

            // Calculate eviction cost based on is_last_copies and the victim cacheinfo
            DeltaReward tmp_eviction_cost = edge_wrapper_ptr->calcLocalCachedReward(tmp_local_cached_popularity, tmp_redirected_cached_popularity, is_last_copies);
            local_eviction_cost += tmp_eviction_cost;
        }

        rwlock_for_victim_tracker_->unlock_shared(context_name);
        return local_eviction_cost;
    }

    uint64_t VictimTracker::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock to get cache size usage atomically (TODO: maybe NO need to acquire a read lock for size_bytes_ here)
        std::string context_name = "VictimTracker::getSizeForCapacity()";
        rwlock_for_victim_tracker_->acquire_lock_shared(context_name);

        uint64_t total_size = size_bytes_;

        rwlock_for_victim_tracker_->unlock_shared(context_name);

        return total_size;
    }

    // Utils

    // For victim synchronization

    SeqNum VictimTracker::getAndIncrCurSeqnum_(const uint32_t& dst_edge_idx_for_compression) const
    {
        // NOTE: NO need to acquire a write lock which has been done in getLocalVictimSyncsetForSynchronization()

        SeqNum cur_seqnum = 0; // Cur seqnum starts from 0 to synchronize local victim info to each dst neighbor
        const bool is_valid_victimsync_monitor = peredge_victimsync_monitor_[dst_edge_idx_for_compression].isValid();
        if (!is_valid_victimsync_monitor)
        {
            peredge_victimsync_monitor_[dst_edge_idx_for_compression].validate(); // Next local victim syncset should be cur_seqnum + 1

            //size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint32_t)); // dst edge idx
            size_bytes_ = Util::uint64Add(size_bytes_, peredge_victimsync_monitor_[dst_edge_idx_for_compression].getSizeForCapacity()); // cur seqnum
        }
        else
        {
            cur_seqnum = peredge_victimsync_monitor_[dst_edge_idx_for_compression].getCurSeqnum();
        }

        assert(peredge_victimsync_monitor_[dst_edge_idx_for_compression].isValid());
        peredge_victimsync_monitor_[dst_edge_idx_for_compression].incrCurSeqnum(); // Next local victim syncset should be cur_seqnum + 1

        return cur_seqnum;
    }

    bool VictimTracker::checkAndResetNeedEnforcement_(const uint32_t& dst_edge_idx_for_compression) const
    {
        // NOTE: NO need to acquire a write lock which has been done in getLocalVictimSyncsetForSynchronization()

        bool need_enforcement = false;
        const bool is_valid_victimsync_monitor = peredge_victimsync_monitor_[dst_edge_idx_for_compression].isValid();
        assert(is_valid_victimsync_monitor); // NOTE: getAndIncrCurSeqnum_() has inserted VictimsyncMonitor for dst_edge_idx_for_compression if not exist, and we will NEVER remove it

        need_enforcement = peredge_victimsync_monitor_[dst_edge_idx_for_compression].needEnforcement();
        if (need_enforcement)
        {
            peredge_victimsync_monitor_[dst_edge_idx_for_compression].resetEnforcement();
        }

        return need_enforcement;
    }

    VictimSyncset VictimTracker::getVictimSyncset_(const uint32_t& edge_idx, const SeqNum& seqnum, const bool& is_enforce_complete) const
    {
        // NOTE: NO need to acquire a read lock which has been done in getLocalVictimSyncsetForSynchronization() and updateForNeighborVictimSyncset()

        // Get cache margin bytes and victim cacheinfos of edge idx
        uint64_t cache_margin_bytes = 0;
        std::list<VictimCacheinfo> synced_victims;
        const bool is_valid_victim_metadata = peredge_victim_metadata_[edge_idx].isValid();
        if (is_valid_victim_metadata)
        {
            cache_margin_bytes = peredge_victim_metadata_[edge_idx].getCacheMarginBytes();
            synced_victims = peredge_victim_metadata_[edge_idx].getVictimCacheinfos();
        }

        // Get beaconed victims of edge idx
        std::list<std::pair<Key, DirinfoSet>> beaconed_victims;
        for (perkey_victim_dirinfo_t::const_iterator dirinfo_list_iter = perkey_victim_dirinfo_.begin(); dirinfo_list_iter != perkey_victim_dirinfo_.end(); dirinfo_list_iter++)
        {
            if (dirinfo_list_iter->second.getBeaconEdgeIdx() == edge_idx)
            {
                const Key& tmp_key = dirinfo_list_iter->first;
                const DirinfoSet& tmp_dirinfo_set = dirinfo_list_iter->second.getDirinfoSetRef(); // NOTE: dirinfo set from victim dirinfo MUST be complete
                beaconed_victims.push_back(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        VictimSyncset victim_syncset(seqnum, is_enforce_complete, cache_margin_bytes, synced_victims, beaconed_victims);
        assert(victim_syncset.isComplete()); // NOTE: we get complete victim syncset here (dedup/delta-compression is performed outside VictimSyncset, i.e., in CoveredCacheManager)

        return victim_syncset;
    }

    bool VictimTracker::replacePrevVictimSyncset_(const uint32_t& dst_edge_idx_for_compression, const VictimSyncset& current_victim_syncset, VictimSyncset& prev_victim_syncset) const
    {
        checkPointers_();

        // NOTE: NO need to acquire a write lock to replace prev victim syncset, which has been done by getLocalVictimSyncsetForSynchronization()

        assert(current_victim_syncset.isComplete()); // NOTE: current victim syncset is generated by getVictimSyncset_(), which MUST be complete

        const bool is_valid_victimsync_monitor = peredge_victimsync_monitor_[dst_edge_idx_for_compression].isValid();
        assert(is_valid_victimsync_monitor); // NOTE: getAndIncrCurSeqnum_() has inserted VictimsyncMonitor for dst_edge_idx_for_compression if not exist, and we will NEVER remove it

        bool is_prev_victim_syncset_exist = peredge_victimsync_monitor_[dst_edge_idx_for_compression].getPrevVictimSyncset(prev_victim_syncset); // Get prev victim syncset if any
        if (is_prev_victim_syncset_exist) // Prev victim syncset already exists
        {
            peredge_victimsync_monitor_[dst_edge_idx_for_compression].setPrevVictimSyncset(current_victim_syncset); // Replace prev victim syncset with current victim syncset

            // Update cache size usage for replaced victim syncset in peredge_prev_victim_syncset_
            uint64_t current_victim_syncset_size_bytes = current_victim_syncset.getSizeForCapacity();
            uint64_t prev_victim_syncset_size_bytes = prev_victim_syncset.getSizeForCapacity();
            if (current_victim_syncset_size_bytes > prev_victim_syncset_size_bytes)
            {
                size_bytes_ = Util::uint64Add(size_bytes_, Util::uint64Minus(current_victim_syncset_size_bytes, prev_victim_syncset_size_bytes));
            }
            else
            {
                size_bytes_ = Util::uint64Minus(size_bytes_, Util::uint64Minus(prev_victim_syncset_size_bytes, current_victim_syncset_size_bytes));
            }

            assert(prev_victim_syncset.isComplete()); // NOTE: prev victim syncset is replaced by complete victim syncset generated before, which also MUST be complete
        }
        else
        {
            peredge_victimsync_monitor_[dst_edge_idx_for_compression].setPrevVictimSyncset(current_victim_syncset); // Insert current victim syncset as prev victim syncset

            // Update cache size usage for new victim syncset in peredge_prev_victim_syncset_
            uint64_t current_victim_syncset_size_bytes = current_victim_syncset.getSizeForCapacity();
            size_bytes_ = Util::uint64Add(size_bytes_, current_victim_syncset_size_bytes);
        }

        return is_prev_victim_syncset_exist;
    }

    // For victim update and removal

    void VictimTracker::replaceVictimMetadataForEdgeIdx_(const uint32_t& edge_idx, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& synced_victim_cacheinfos, const CooperationWrapperBase* cooperation_wrapper_ptr)
    {
        // NOTE: NO need to acquire a write lock which has been done in updateLocalSyncedVictims() and updateForNeighborVictimSyncset()

        assert(synced_victim_cacheinfos.size() <= peredge_synced_victimcnt_);
        assert(cooperation_wrapper_ptr != NULL);

        // Update cacheinfos of synced victims for the specific edge node
        std::list<VictimCacheinfo> old_synced_victim_cacheinfos;
        const bool is_valid_victim_metadata = peredge_victim_metadata_[edge_idx].isValid();
        if (!is_valid_victim_metadata) // No synced victims for the specific edge node
        {
            // (OBSOLETE: passed-in local/neighbor synced victim cacheinfos MUST be complete) NOTE: even if all victim cacheinfos are removed, we will NOT erase the edge-level victim metadata (see VictimTracker::removeVictimsForGivenEdge()) -> no edge-level victim metadata indicates the first victim synchronization of the edge idx, which MUST transmit complete victim cacheinfos

            // Add latest synced victims for the specific edge node
            // NOTE: EdgelevelVictimMetadata will assert that all victim cacheinfos are complete
            peredge_victim_metadata_[edge_idx] = EdgelevelVictimMetadata(cache_margin_bytes, synced_victim_cacheinfos);
            assert(peredge_victim_metadata_[edge_idx].isValid());

            //size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint32_t)); // For edge_idx
            size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint64_t)); // For cache_margin_bytes
        }
        else // Current edge node has tracked some local synced victims before
        {
            // Preserve old local synced victim cacheinfos (MUST be complete) if any
            old_synced_victim_cacheinfos = peredge_victim_metadata_[edge_idx].getVictimCacheinfos();

            // Replace old local synced victim cacheinfos with the latest ones
            // NOTE: EdgelevelVictimMetadata will assert that all victim cacheinfos are complete
            peredge_victim_metadata_[edge_idx] = EdgelevelVictimMetadata(cache_margin_bytes, synced_victim_cacheinfos);
        }

        // Update victim dirinfos for new synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator new_cacheinfo_list_iter = synced_victim_cacheinfos.begin(); new_cacheinfo_list_iter != synced_victim_cacheinfos.end(); new_cacheinfo_list_iter++)
        {
            // Local/neighbor synced victim cacheinfos passed into victim tracker MUST be complete
            assert(new_cacheinfo_list_iter->isComplete());

            size_bytes_ = Util::uint64Add(size_bytes_, new_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each latest local synced victims

            const Key& tmp_key = new_cacheinfo_list_iter->getKey();
            //bool tmp_is_local_beaconed = (local_beaconed_synced_victim_dirinfosets.find(tmp_key) != local_beaconed_synced_victim_dirinfosets.end());
            uint32_t tmp_beacon_edge_idx = cooperation_wrapper_ptr->getBeaconEdgeIdx(tmp_key);
            perkey_victim_dirinfo_t::iterator dirinfo_list_iter = KVListHelper<Key, VictimDirinfo>::findVFromListForK(tmp_key, perkey_victim_dirinfo_);
            if (dirinfo_list_iter == perkey_victim_dirinfo_.end()) // No victim dirinfo for the key
            {
                // Add a new victim dirinfo for the key
                //perkey_victim_dirinfo_.push_back(std::pair(tmp_key, VictimDirinfo(tmp_is_local_beaconed)));
                perkey_victim_dirinfo_.push_back(std::pair(tmp_key, VictimDirinfo(tmp_beacon_edge_idx)));
                dirinfo_list_iter = perkey_victim_dirinfo_.end();
                dirinfo_list_iter--; // Point to the last element just pushed back
                assert(dirinfo_list_iter != perkey_victim_dirinfo_.end());

                dirinfo_list_iter->second.incrRefcnt();

                size_bytes_ = Util::uint64Add(size_bytes_, (tmp_key.getKeyLength() + dirinfo_list_iter->second.getSizeForCapacity())); // For each inserted victim dirinfo of a new synced victim
            }
            else // The key already has a victim dirinfo
            {
                //assert(dirinfo_list_iter->second.isLocalBeaconed() == tmp_is_local_beaconed);
                assert(dirinfo_list_iter->second.getBeaconEdgeIdx() == tmp_beacon_edge_idx);

                dirinfo_list_iter->second.incrRefcnt();
            }
        }

        // Remove victim dirinfos for old synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator old_cacheinfo_list_iter = old_synced_victim_cacheinfos.begin(); old_cacheinfo_list_iter != old_synced_victim_cacheinfos.end(); old_cacheinfo_list_iter++)
        {
            assert(old_cacheinfo_list_iter->isComplete()); // NOTE: old victim cacheinfo stored in victim tracker MUST be complete

            size_bytes_ = Util::uint64Minus(size_bytes_, old_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each old synced victim

            const Key& tmp_key = old_cacheinfo_list_iter->getKey();
            tryToReleaseVictimDirinfo_(tmp_key);
        }

        return;
    }

    void VictimTracker::replaceVictimDirinfoSets_(const std::list<std::pair<Key, DirinfoSet>>& beaconed_synced_victim_dirinfosets, const bool& is_local_beaconed)
    {
        // NOTE: NO need to acquire a write lock which has been done in updateLocalSyncedVictims() and updateForNeighborVictimSyncset()

        // Update victim dirinfos for beaconed victims
        for (std::list<std::pair<Key, DirinfoSet>>::const_iterator beaconed_dirinfosets_list_iter = beaconed_synced_victim_dirinfosets.begin(); beaconed_dirinfosets_list_iter != beaconed_synced_victim_dirinfosets.end(); beaconed_dirinfosets_list_iter++)
        {
            assert(beaconed_dirinfosets_list_iter->second.isComplete()); // NOTE: local/neighbor beaconed victim dirinfosets MUST be complete

            const Key& tmp_key = beaconed_dirinfosets_list_iter->first;
            perkey_victim_dirinfo_t::iterator dirinfo_list_iter = KVListHelper<Key, VictimDirinfo>::findVFromListForK(tmp_key, perkey_victim_dirinfo_);

            if (is_local_beaconed) // If dirinfosets belong to keys locally beaconed by the current edge node
            {
                // NOTE: VictimDirinfo MUST exist for local beaconed victims (local/neighbor synced victims from local-edge-cache/received-victim-syncset have already been added in replaceVictimCacheinfosForEdgeIdx_())
                assert(dirinfo_list_iter != perkey_victim_dirinfo_.end());
            }

            if (dirinfo_list_iter != perkey_victim_dirinfo_.end())
            {
                if (is_local_beaconed)
                {
                    //assert(dirinfo_map_iter->second.isLocalBeaconed() == true); // NOTE: we have set is local beaconed in VictimTracker::replaceVictimMetadataForEdgeIdx_()
                    //dirinfo_map_iter->second.markLocalBeaconed();

                    assert(dirinfo_list_iter->second.getBeaconEdgeIdx() == edge_idx_); // NOTE: we have set beacon edge idx in VictimTracker::replaceVictimMetadataForEdgeIdx_()
                    //dirinfo_map_iter->second.setBeaconEdgeIdx(edge_idx_);
                }

                uint64_t prev_victim_dirinfo_size_bytes = dirinfo_list_iter->second.getSizeForCapacity();
                dirinfo_list_iter->second.setDirinfoSet(beaconed_dirinfosets_list_iter->second);
                uint64_t cur_victim_dirinfo_size_bytes = dirinfo_list_iter->second.getSizeForCapacity();

                if (prev_victim_dirinfo_size_bytes < cur_victim_dirinfo_size_bytes)
                {
                    size_bytes_ = Util::uint64Add(size_bytes_, (cur_victim_dirinfo_size_bytes - prev_victim_dirinfo_size_bytes));
                }
                else if (prev_victim_dirinfo_size_bytes > cur_victim_dirinfo_size_bytes)
                {
                    size_bytes_ = Util::uint64Minus(size_bytes_, (prev_victim_dirinfo_size_bytes - cur_victim_dirinfo_size_bytes));
                }
                // NOTE: NO need to update size_bytes_ if prev_victim_dirinfo_size_bytes == cur_victim_dirinfo_size_bytes
            }
        }

        return;
    }

    void VictimTracker::tryToReleaseVictimDirinfo_(const Key& key)
    {
        // NOTE: NO need to acquire a write lock which has been done in removeVictimsForGivenEdge(), and replaceVictimMetadataForEdgeIdx_() (from updateLocalSyncedVictims() and updateForNeighborVictimSyncset())

        perkey_victim_dirinfo_t::iterator dirinfo_list_iter = KVListHelper<Key, VictimDirinfo>::findVFromListForK(key, perkey_victim_dirinfo_);
        if (dirinfo_list_iter != perkey_victim_dirinfo_.end()) // NOTE: key may NOT have corresponding victim dirinfo if the beacon node has NOT synced victim dirinfo
        {
            dirinfo_list_iter->second.decrRefcnt();

            if (dirinfo_list_iter->second.getRefcnt() == 0) // No edge node is tracking the key as a synced victim
            {
                size_bytes_ = Util::uint64Minus(size_bytes_, (key.getKeyLength() + dirinfo_list_iter->second.getSizeForCapacity())); // For each erased victim dirinfo of an old synced victim

                // Remove the victim dirinfo for the key
                perkey_victim_dirinfo_.erase(dirinfo_list_iter);
            }
        }
        return;
    }

    // For trade-off-aware placement calculation

    void VictimTracker::findVictimsForPlacement_(const ObjectSize& object_size, const Edgeset& placement_edgeset, std::list<std::pair<Key, Edgeset>>& pervictim_edgeset, std::list<std::pair<Key, std::list<VictimCacheinfo>>>& pervictim_cacheinfos, std::list<std::pair<uint32_t, std::list<Key>>>& peredge_synced_victimset, std::list<std::pair<uint32_t, std::list<Key>>>& peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::list<std::pair<uint32_t, std::list<VictimCacheinfo>>>& extra_peredge_victim_cacheinfos) const
    {
        // NOTE: NO need to acquire a read lock which has been done in calcEvictionCost()

        pervictim_edgeset.clear();
        pervictim_cacheinfos.clear();
        peredge_synced_victimset.clear();
        peredge_fetched_victimset.clear();
        victim_fetch_edgeset.clear();

        const bool with_extra_victims = (extra_peredge_victim_cacheinfos.size() > 0);

        // Enumerate each involved edge node to find victims
        for (std::unordered_set<uint32_t>::const_iterator placement_edgeset_const_iter = placement_edgeset.begin(); placement_edgeset_const_iter != placement_edgeset.end(); placement_edgeset_const_iter++)
        {
            uint32_t tmp_edge_idx = *placement_edgeset_const_iter;

            const bool tmp_is_valid = peredge_victim_metadata_[tmp_edge_idx].isValid();

            // (MINOR CASE) Non-existing edge-level victim metadata indicates empty victim syncset carried by the request triggering placement calculation, i.e., system is just starting and tmp_edge_idx has NOT cached any object
            if (!tmp_is_valid)
            {
                continue; // Equivalent to that tmp_edge_idx has NOT used any cache size (i.e., sufficiently large cache margin bytes) and hence NOT find any victim from tmp_edge_idx
            }

            // NOTE: from here, edge-level victim metadata for tmp_edge_idx MUST exist, as we have performed victim synchronization when collecting local uncached popularity of the given key from tmp_edge_idx
            // NOTE: even if all victim cacheinfos in the edge-level victim metadata are removed after placement calculation, we will NOT erase the edge-level victim metadata from peredge_victim_metadata_
            assert(tmp_is_valid);

            // Prepare extra victim cacheinfos for tmp_edge_idx if any
            std::list<VictimCacheinfo> tmp_extra_victim_cacheinfos;
            if (with_extra_victims)
            {
                std::list<std::pair<uint32_t, std::list<VictimCacheinfo>>>::const_iterator extra_victim_cacheinfos_list_const_iter = KVListHelper<uint32_t, std::list<VictimCacheinfo>>::findVFromListForK(tmp_edge_idx, extra_peredge_victim_cacheinfos);
                assert(extra_victim_cacheinfos_list_const_iter != extra_peredge_victim_cacheinfos.end());
                tmp_extra_victim_cacheinfos = extra_victim_cacheinfos_list_const_iter->second;
            }

            // Find victims in tmp_edge_idx if without sufficient cache space
            const EdgelevelVictimMetadata& tmp_edgelevel_victim_metadata = peredge_victim_metadata_[tmp_edge_idx];
            tmp_edgelevel_victim_metadata.findVictimsForObjectSize(tmp_edge_idx, object_size, pervictim_edgeset, pervictim_cacheinfos, peredge_synced_victimset, peredge_fetched_victimset, victim_fetch_edgeset, tmp_extra_victim_cacheinfos);
        } // End of involved edge nodes

        if (with_extra_victims)
        {
            victim_fetch_edgeset.clear(); // NOTE: we DISABLE recursive victim fetching
        }

        return;
    }

    bool VictimTracker::isLastCopiesForVictimEdgeset_(const Key& key, const Edgeset& victim_edgeset, const std::list<std::pair<Key, DirinfoSet>>& extra_perkey_victim_dirinfoset) const
    {
        // NOTE: NO need to acquire a read lock which has been done in calcEvictionCost()
        
        bool is_last_copies = true; // Whether the victim edgeset is the last copies of the given key, i.e., contains all dirinfos

        std::vector<DirinfoSet> tmp_dirinfo_sets;

        // Get synced victim dirinfo set of perkey_victim_dirinfo_ for the given key if any
        perkey_victim_dirinfo_t::const_iterator victim_dirinfo_list_const_iter = KVListHelper<Key, VictimDirinfo>::findVFromListForK(key, perkey_victim_dirinfo_);
        if (victim_dirinfo_list_const_iter != perkey_victim_dirinfo_.end()) // NOTE: synced/extra-fetched victim key may be neighbor-beaconed and NOT have synced victim dirinfo in the current edge node, as the correpsonding beacon node may have NOT synced the latest victim dirinfo to the current edge node
        {
            const VictimDirinfo& tmp_victim_dirinfo = victim_dirinfo_list_const_iter->second;
            DirinfoSet tmp_dirinfo_set = tmp_victim_dirinfo.getDirinfoSetRef();
            assert(tmp_dirinfo_set.isComplete()); // NOTE: victim dirinfo set from victim dirinfo of victim tracker and extra fetched dirinfo set from extra_perkey_victim_dirinfoset MUST be complete

            tmp_dirinfo_sets.push_back(tmp_dirinfo_set);
        }

        // Get extra fetched victim dirinfo set of extra_perkey_victim_dirinfoset for the given key if any
        std::list<std::pair<Key, DirinfoSet>>::const_iterator extra_victim_dirinfo_map_const_iter = KVListHelper<Key, DirinfoSet>::findVFromListForK(key, extra_perkey_victim_dirinfoset);
        if (extra_victim_dirinfo_map_const_iter != extra_perkey_victim_dirinfoset.end()) // NOTE: synced/extra-fetched victim key may be neighbor-beaconed and NOT have extra fetched dirinfo set in extra_perkey_victim_dirinfoset, as the correpsonding beacon node may have NOT synced the latest victim dirinfo to the fetched edge node
        {
            DirinfoSet tmp_dirinfo_set = extra_victim_dirinfo_map_const_iter->second;
            assert(tmp_dirinfo_set.isComplete()); // NOTE: victim dirinfo set from victim dirinfo of victim tracker and extra fetched dirinfo set from extra_perkey_victim_dirinfoset MUST be complete

            tmp_dirinfo_sets.push_back(tmp_dirinfo_set);
        }

        // Check synced/extra-fetched victim dirinfo set
        for (uint32_t i = 0; i < tmp_dirinfo_sets.size(); i++)
        {
            assert(is_last_copies == true); // NOTE: we will break the for loop as long as is_last_copies becomes false

            const DirinfoSet& tmp_dirinfo_set = tmp_dirinfo_sets[i];

            std::list<DirectoryInfo> tmp_dirinfo_list;
            bool with_complete_dirinfo_set = tmp_dirinfo_set.getDirinfoSetIfComplete(tmp_dirinfo_list);
            assert(with_complete_dirinfo_set == true); // NOTE: victim dirinfo set from victim dirinfo of victim tracker and extra fetched dirinfo set from extra_perkey_victim_dirinfoset MUST be complete
            for (std::list<DirectoryInfo>::const_iterator dirinfo_list_const_iter = tmp_dirinfo_list.begin(); dirinfo_list_const_iter != tmp_dirinfo_list.end(); dirinfo_list_const_iter++)
            {
                uint32_t tmp_edge_idx = dirinfo_list_const_iter->getTargetEdgeIdx();
                if (victim_edgeset.find(tmp_edge_idx) == victim_edgeset.end())
                {
                    is_last_copies = false;
                    break;
                }
            }

            if (!is_last_copies)
            {
                break;
            }
        }

        // NOTE: if without synced VicimDirinfo and without extra fetched dirinfo set, beacon server will treat it as the last cache copies conservatively
        return is_last_copies;
    }

    void VictimTracker::checkPointers_() const
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        
        return;
    }
}