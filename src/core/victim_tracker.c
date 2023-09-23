#include "core/victim_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/covered_weight.h"
#include "common/util.h"

namespace covered
{
    // VictimTracker

    const std::string VictimTracker::kClassName = "VictimTracker";

    VictimTracker::VictimTracker(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt) : edge_idx_(edge_idx), peredge_synced_victimcnt_(peredge_synced_victimcnt)
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
        peredge_victim_metadata_.clear();
        perkey_victim_dirinfo_.clear();
    }

    VictimTracker::~VictimTracker()
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        delete rwlock_for_victim_tracker_;
        rwlock_for_victim_tracker_ = NULL;
    }

    // For local synced/beaconed victims

    void VictimTracker::updateLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets)
    {
        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        checkPointers_();

        assert(local_beaconed_local_synced_victim_dirinfosets.size() <= local_synced_victim_cacheinfos.size());

        // Acquire a write lock to update local synced victims atomically
        std::string context_name = "VictimTracker::updateLocalSyncedVictims()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // Replace VictimCacheinfors for local synced victims of current edge node
        replaceVictimMetadataForEdgeIdx_(edge_idx_, local_cache_margin_bytes, local_synced_victim_cacheinfos);

        // Replace VictimDirinfo::dirinfoset for each local beaconed local synced victim of current edge node
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
        perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(key);
        if (dirinfo_map_iter != perkey_victim_dirinfo_.end()) // The beaconed key is a local/neighbor synced victim
        {
            assert(dirinfo_map_iter->second.isLocalBeaconed());
            
            // Update directory info for the key
            bool is_update = dirinfo_map_iter->second.updateDirinfoSet(is_admit, directory_info);

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

    VictimSyncset VictimTracker::getVictimSyncset() const
    {
        checkPointers_();

        // Acquire a read lock to get victim syncset atomically
        std::string context_name = "VictimTracker::getVictimSyncset()";
        rwlock_for_victim_tracker_->acquire_lock_shared(context_name);

        // Get cache margin bytes and local synced victims
        uint64_t local_cache_margin_bytes = 0;
        std::list<VictimCacheinfo> local_synced_victims;
        peredge_victim_metadata_t::const_iterator victim_metadata_map_const_iter = peredge_victim_metadata_.find(edge_idx_);
        if (victim_metadata_map_const_iter != peredge_victim_metadata_.end())
        {
            local_cache_margin_bytes = victim_metadata_map_const_iter->second.getCacheMarginBytes();
            local_synced_victims = victim_metadata_map_const_iter->second.getVictimCacheinfos();
        }

        // Get local beaconed victims
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_victims;
        for (perkey_victim_dirinfo_t::const_iterator dirinfo_map_iter = perkey_victim_dirinfo_.begin(); dirinfo_map_iter != perkey_victim_dirinfo_.end(); dirinfo_map_iter++)
        {
            if (dirinfo_map_iter->second.isLocalBeaconed())
            {
                const Key& tmp_key = dirinfo_map_iter->first;
                const dirinfo_set_t& tmp_dirinfo_set = dirinfo_map_iter->second.getDirinfoSetRef();
                local_beaconed_victims.insert(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        VictimSyncset victim_syncset(local_cache_margin_bytes, local_synced_victims, local_beaconed_victims);

        rwlock_for_victim_tracker_->unlock_shared(context_name);

        return victim_syncset;
    }

    void VictimTracker::updateForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets)
    {
        // NOTE: limited computation overhead to update neighbor synced victim infos, as we track limited number of neighbor synced victims for the specific edge node

        checkPointers_();

        const uint64_t neighbor_cache_margin_bytes = victim_syncset.getCacheMarginBytes();
        const std::list<VictimCacheinfo>& neighbor_synced_victim_cacheinfos = victim_syncset.getLocalSyncedVictimsRef();
        assert(local_beaconed_neighbor_synced_victim_dirinfosets.size() <= neighbor_synced_victim_cacheinfos.size());

        // Acquire a write lock to update local synced victims atomically
        std::string context_name = "VictimTracker::updateForVictimSyncset()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);
        
        // Replace VictimCacheinfos for neighbor synced victims of the given edge node
        replaceVictimMetadataForEdgeIdx_(source_edge_idx, neighbor_cache_margin_bytes, neighbor_synced_victim_cacheinfos);

        // Try to replace VictimDirinfo::dirinfoset (if any) for each neighbor beaconed neighbor synced victim of the given edge node
        const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& neighbor_beaconed_victim_dirinfosets = victim_syncset.getLocalBeaconedVictimsRef();
        replaceVictimDirinfoSets_(neighbor_beaconed_victim_dirinfosets, false);

        // Replace VictimDirinfo::dirinfoset for each local beaconed neighbor synced victim of the given edge node
        replaceVictimDirinfoSets_(local_beaconed_neighbor_synced_victim_dirinfosets, true);

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    // For trade-off-aware placement calculation
    
    DeltaReward VictimTracker::calcEvictionCost(const ObjectSize& object_size, const std::unordered_set<uint32_t>& placement_edgeset) const
    {
        checkPointers_();
        
        // Acquire a read lock to calculate eviction cost atomically
        std::string context_name = "VictimTracker::calcEvictionCost()";
        rwlock_for_victim_tracker_->acquire_lock_shared(context_name);

        DeltaReward eviction_cost = 0.0;

        // Get weight parameters from static class atomically
        const WeightInfo weight_info = CoveredWeight::getWeightInfo();
        const Weight local_hit_weight = weight_info.getLocalHitWeight();
        const Weight cooperative_hit_weight = weight_info.getCooperativeHitWeight();

        // Find victims from placement edgeset if admit a hot object with the given size
        std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher> pervictim_edgeset;
        std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher> pervictim_cacheinfos;
        findVictimsForPlacement_(object_size, placement_edgeset, pervictim_edgeset, pervictim_cacheinfos);

        // Enumerate found victims to calculate eviction cost
        for (std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>::const_iterator pervictim_edgeset_const_iter = pervictim_edgeset.begin(); pervictim_edgeset_const_iter != pervictim_edgeset.end(); pervictim_edgeset_const_iter++)
        {
            // Check if the victim edgeset is the last copies of the given key
            const Key& tmp_victim_key = pervictim_edgeset_const_iter->first;
            const std::unordered_set<uint32_t>& tmp_victim_edgeset = pervictim_edgeset_const_iter->second;
            bool is_last_copies = isLastCopiesForVictimEdgeset_(tmp_victim_key, tmp_victim_edgeset);

            // NOTE: we add each pair of edgeidx and cacheinfo of a victim simultaneously in findVictimsForPlacement_() -> tmp_victim_cacheinfos MUST exist and has the same size as tmp_victim_edgeset
            std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>::const_iterator pervictim_cacheinfos_const_iter = pervictim_cacheinfos.find(tmp_victim_key);
            assert(pervictim_cacheinfos_const_iter != pervictim_cacheinfos.end());
            const std::list<VictimCacheinfo>& tmp_victim_cacheinfos = pervictim_cacheinfos_const_iter->second;
            assert(tmp_victim_cacheinfos.size() == tmp_victim_edgeset.size());

            // Calculate eviction cost based on is_last_copies and tmp_victim_cacheinfos
            for (std::list<VictimCacheinfo>::const_iterator victim_cacheinfo_list_const_iter = tmp_victim_cacheinfos.begin(); victim_cacheinfo_list_const_iter != tmp_victim_cacheinfos.end(); victim_cacheinfo_list_const_iter++)
            {
                if (is_last_copies) // All local/redirected cache hits become global cache misses
                {
                    eviction_cost += victim_cacheinfo_list_const_iter->getLocalCachedPopularity() * local_hit_weight; // w1
                    eviction_cost += victim_cacheinfo_list_const_iter->getRedirectedCachedPopularity() * cooperative_hit_weight; // w2
                }
                else // Local cache hits become redirected cache hits
                {
                    eviction_cost += victim_cacheinfo_list_const_iter->getLocalCachedPopularity() * (local_hit_weight - cooperative_hit_weight); // w1 - w2
                }
            }
        }

        rwlock_for_victim_tracker_->unlock_shared(context_name);
        return eviction_cost;
    }

    // For non-blocking placement deployment

    void VictimTracker::removeVictimsForGivenEdge(const uint32_t& edge_idx, const std::unordered_set<Key, KeyHasher>& victim_keyset)
    {
        checkPointers_();

        // Acquire a write lock to remove victims atomically
        std::string context_name = "VictimTracker::removeVictimsForGivenEdge()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // NOTE: each edge node in placement edgeset MUST have EdgeLevelVictimMetadata
        peredge_victim_metadata_t::iterator peredge_victim_metadata_iter = peredge_victim_metadata_.find(edge_idx);
        assert(peredge_victim_metadata_iter != peredge_victim_metadata_.end());

        peredge_victim_metadata_iter->second.removeVictimsForPlacement(victim_keyset);

        rwlock_for_victim_tracker_->unlock(context_name);
        return;
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

    void VictimTracker::replaceVictimMetadataForEdgeIdx_(const uint32_t& edge_idx, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& synced_victim_cacheinfos)
    {
        // NOTE: NO need to acquire a write lock which has been done in updateLocalSyncedVictims() and updateForVictimSyncset()

        assert(synced_victim_cacheinfos.size() <= peredge_synced_victimcnt_);

        // Update cacheinfos of synced victims for the specific edge node
        std::list<VictimCacheinfo> old_synced_victim_cacheinfos;
        peredge_victim_metadata_t::iterator victim_metadata_map_iter = peredge_victim_metadata_.find(edge_idx);
        if (victim_metadata_map_iter == peredge_victim_metadata_.end()) // No synced victims for the specific edge node
        {
            // Add latest synced victims for the specific edge node
            victim_metadata_map_iter = peredge_victim_metadata_.insert(std::pair(edge_idx, EdgelevelVictimMetadata(cache_margin_bytes, synced_victim_cacheinfos))).first;
            assert(victim_metadata_map_iter != peredge_victim_metadata_.end());

            size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint32_t)); // For edge_idx
            size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint64_t)); // For cache_margin_bytes
        }
        else // Current edge node has tracked some local synced victims before
        {
            // Preserve old local synced victim cacheinfos if any
            old_synced_victim_cacheinfos = victim_metadata_map_iter->second.getVictimCacheinfos();

            // Replace old local synced victim cacheinfos with the latest ones
            victim_metadata_map_iter->second = EdgelevelVictimMetadata(cache_margin_bytes, synced_victim_cacheinfos);
        }

        // Update victim dirinfos for new synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator new_cacheinfo_list_iter = synced_victim_cacheinfos.begin(); new_cacheinfo_list_iter != synced_victim_cacheinfos.end(); new_cacheinfo_list_iter++)
        {
            size_bytes_ = Util::uint64Add(size_bytes_, new_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each latest local synced victims

            const Key& tmp_key = new_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            if (dirinfo_map_iter == perkey_victim_dirinfo_.end()) // No victim dirinfo for the key
            {
                // Add a new victim dirinfo for the key
                dirinfo_map_iter = perkey_victim_dirinfo_.insert(std::pair(tmp_key, VictimDirinfo())).first;
                assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

                dirinfo_map_iter->second.incrRefcnt();

                size_bytes_ = Util::uint64Add(size_bytes_, (tmp_key.getKeyLength() + dirinfo_map_iter->second.getSizeForCapacity())); // For each inserted victim dirinfo of a new synced victim
            }
            else // The key already has a victim dirinfo
            {
                dirinfo_map_iter->second.incrRefcnt();
            }
        }

        // Remove victim dirinfos for old synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator old_cacheinfo_list_iter = old_synced_victim_cacheinfos.begin(); old_cacheinfo_list_iter != old_synced_victim_cacheinfos.end(); old_cacheinfo_list_iter++)
        {
            size_bytes_ = Util::uint64Minus(size_bytes_, old_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each old synced victim

            const Key& tmp_key = old_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

            dirinfo_map_iter->second.decrRefcnt();
            if (dirinfo_map_iter->second.getRefcnt() == 0) // No edge node is tracking the key as a synced victim
            {
                size_bytes_ = Util::uint64Minus(size_bytes_, (tmp_key.getKeyLength() + dirinfo_map_iter->second.getSizeForCapacity())); // For each erased victim dirinfo of an old synced victim

                // Remove the victim dirinfo for the key
                perkey_victim_dirinfo_.erase(dirinfo_map_iter);
            }
        }

        return;
    }

    void VictimTracker::replaceVictimDirinfoSets_(const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& beaconed_synced_victim_dirinfosets, const bool& is_local_beaconed)
    {
        // Update victim dirinfos for beaconed victims
        for (std::unordered_map<Key, dirinfo_set_t, KeyHasher>::const_iterator beaconed_dirinfosets_map_iter = beaconed_synced_victim_dirinfosets.begin(); beaconed_dirinfosets_map_iter != beaconed_synced_victim_dirinfosets.end(); beaconed_dirinfosets_map_iter++)
        {
            const Key& tmp_key = beaconed_dirinfosets_map_iter->first;
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);

            if (is_local_beaconed) // If dirinfosets belong to keys locally beaconed by the current edge node
            {
                // NOTE: VictimDirinfo MUST exist for local beaconed victims (local/neighbor synced victims from local-edge-cache/received-victim-syncset have already been added in replaceVictimCacheinfosForEdgeIdx_())
                assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());
            }

            if (dirinfo_map_iter != perkey_victim_dirinfo_.end())
            {
                if (is_local_beaconed)
                {
                    dirinfo_map_iter->second.markLocalBeaconed();
                }

                uint64_t prev_victim_dirinfo_size_bytes = dirinfo_map_iter->second.getSizeForCapacity();
                dirinfo_map_iter->second.setDirinfoSet(beaconed_dirinfosets_map_iter->second);
                uint64_t cur_victim_dirinfo_size_bytes = dirinfo_map_iter->second.getSizeForCapacity();

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

    void VictimTracker::findVictimsForPlacement_(const ObjectSize& object_size, const std::unordered_set<uint32_t>& placement_edgeset, std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos) const
    {
        pervictim_edgeset.clear();
        pervictim_cacheinfos.clear();

        // Enumerate each involved edge node to find victims
        for (std::unordered_set<uint32_t>::const_iterator placement_edgeset_const_iter = placement_edgeset.begin(); placement_edgeset_const_iter != placement_edgeset.end(); placement_edgeset_const_iter++)
        {
            uint32_t tmp_edge_idx = *placement_edgeset_const_iter;

            // NOTE: edge-level victim metadata for tmp_edge_idx MUST exist, as we have performed victim synchronization when collecting local uncached popularity of the given key from tmp_edge_idx
            // NOTE: even if all victim cacheinfos in the edge-level victim metadata are removed after placement calculation, we will NOT erase the edge-level victim metadata from peredge_victim_metadata_
            peredge_victim_metadata_t::const_iterator victim_metadata_map_const_iter = peredge_victim_metadata_.find(tmp_edge_idx);
            assert(victim_metadata_map_const_iter != peredge_victim_metadata_.end());

            // Find victims in tmp_edge_idx if without sufficient cache space
            const EdgelevelVictimMetadata& tmp_edgelevel_victim_metadata = victim_metadata_map_const_iter->second;
            bool need_more_victims = tmp_edgelevel_victim_metadata.findVictimsForObjectSize(tmp_edge_idx, object_size, pervictim_edgeset, pervictim_cacheinfos);

            // TODO: proactive fetching
            // if (need_more_victims) {}
            
        } // End of involved edge nodes

        return;
    }

    bool VictimTracker::isLastCopiesForVictimEdgeset_(const Key& key, const std::unordered_set<uint32_t>& victim_edgeset) const
    {
        bool is_last_copies = true; // Whether the victim edgeset is the last copies of the given key, i.e., contains all dirinfos

        perkey_victim_dirinfo_t::const_iterator victim_dirinfo_map_const_iter = perkey_victim_dirinfo_.find(key);
        if (victim_dirinfo_map_const_iter == perkey_victim_dirinfo_.end()) // NOTE: victim key may NOT have victim dirinfo in the current edge node, as the correpsonding beacon node may have NOT synced the latest victim dirinfo to the current edge node
        {
            is_last_copies = true; // NOTE: if without VicimDirinfo, beacon server will treat it as the last cache copies conservatively
        }
        else
        {
            const VictimDirinfo& tmp_victim_dirinfo = victim_dirinfo_map_const_iter->second;
            const dirinfo_set_t& tmp_dirinfo_set_ref = tmp_victim_dirinfo.getDirinfoSetRef();
            for (dirinfo_set_t::const_iterator dirinfo_set_const_iter = tmp_dirinfo_set_ref.begin(); dirinfo_set_const_iter != tmp_dirinfo_set_ref.end(); dirinfo_set_const_iter++)
            {
                uint32_t tmp_edge_idx = dirinfo_set_const_iter->getTargetEdgeIdx();
                if (victim_edgeset.find(tmp_edge_idx) == victim_edgeset.end())
                {
                    is_last_copies = false;
                    break;
                }
            }
        }

        return is_last_copies;
    }

    void VictimTracker::checkPointers_() const
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        
        return;
    }
}