#include "core/victim_tracker.h"

#include <assert.h>
#include <set>
#include <sstream>

namespace covered
{
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

        perkey_synced_victims_.clear();
        peredge_victim_iters_.clear();
    }

    VictimTracker::~VictimTracker()
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        delete rwlock_for_victim_tracker_;
        rwlock_for_victim_tracker_ = NULL;
    }

    // TODO: END HERE
    void VictimTracker::updateLocalSyncedVictimInfos(const std::vector<Key>& local_synced_victim_keys, const std::vector<SyncedVictim>& local_synced_victims)
    {
        checkPointers_();

        assert(local_synced_victim_keys.size() == local_synced_victims.size());
        assert(local_synced_victim_keys.size() <= peredge_synced_victimcnt_);

        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        // Pop old local synced victims if any
        perkey_synced_victims_t old_perkey_local_synced_victims;
        peredge_victim_iters_t::iterator peredge_map_iter = peredge_victim_iters_.find(edge_idx_);
        if (peredge_map_iter != peredge_victim_iters_.end()) // Current edge node has tracked some local synced victims before
        {
            std::vector<perkey_synced_victims_t::iterator>& old_local_victim_iters = peredge_map_iter->second;
            for (uint32_t old_local_victim_idx = 0; old_local_victim_idx < old_local_victim_iters.size(); old_local_victim_idx++)
            {
                perkey_synced_victims_t::iterator& old_local_victim_iter = old_local_victim_iters[old_local_victim_idx];

                // Temporarily preserve old local synced victim
                old_perkey_local_synced_victims.insert(std::pair(old_local_victim_iter->first, old_local_victim_iter->second));

                // Remove old local synced victim if refcnt becomes zero
                bool is_refcnt_zero = old_local_victim_iter->second.decrRefcnt();
                if (is_refcnt_zero)
                {
                    perkey_synced_victims_.erase(old_local_victim_iter);
                }
            }

            // Clear old local synced victim iterators
            peredge_map_iter->second.clear();
        }
        else // No old local synced victims for the current edge node
        {
            // Add an empty vector of iterators for the current edge node
            peredge_map_iter = peredge_victim_iters_.insert(std::pair(edge_idx_, std::vector<perkey_synced_victims_t::iterator>())).first;
            assert(peredge_map_iter != peredge_victim_iters_.end());
        }
        assert(peredge_map_iter->second.empty());

        // Update perkey_synced_victims_
        for (uint32_t local_synced_victim_idx = 0; local_synced_victim_idx < local_synced_victim_keys.size(); local_synced_victim_idx++)
        {
            const Key& tmp_key = local_synced_victim_keys[local_synced_victim_idx];
            const SyncedVictim& tmp_synced_victim = local_synced_victims[local_synced_victim_idx];

            perkey_synced_victims_t::iterator tmp_victim_iter = perkey_synced_victims_.find(tmp_key);
            if (tmp_victim_iter == perkey_synced_victims_.end()) // Key is not a local synced victim before
            {
                tmp_victim_iter = perkey_synced_victims_.insert(std::pair(tmp_key, tmp_synced_victim)).first;
                assert(tmp_victim_iter != perkey_synced_victims_.end());
            }
            else
            {
                tmp_victim_iter->second = tmp_synced_victim;
            }
        }

        // Check synced victims of current edge node
        peredge_synced_victims_t::iterator map_iter = peredge_synced_victims_.find(edge_idx_);
        if (map_iter == peredge_synced_victims_.end()) // No local synced victims for the current edge node
        {
            map_iter = peredge_synced_victims_.insert(std::pair(edge_idx_, std::list<SyncedVictim>())).first;
            assert(map_iter != peredge_synced_victims_.end());

            for (std::list<VictimInfo>::const_iterator list_iter = local_synced_victim_infos.begin(); list_iter != local_synced_victim_infos.end(); list_iter++)
            {
                map_iter->second.push_back(SyncedVictim(*list_iter, std::set<DirectoryInfo, DirectoryInfoHasher>())); // Use empty DirectoryInfo for newly inserted SyncedVictim
            }
        }
        else // Current edge node has tracked some local synced victims before
        {
            // Replace old local synced victims with an empty list
            std::list<SyncedVictim> old_local_synced_victims = map_iter->second;
            map_iter->second = std::list<SyncedVictim>();

            for (std::list<VictimInfo>::const_iterator list_iter = local_synced_victim_infos.begin(); list_iter != local_synced_victim_infos.end(); list_iter++)
            {
                // Keep DirectoryInfo for the current local synced victim if any
                std::set<DirectoryInfo, DirectoryInfoHasher> tmp_dirinfo_set;
                for (std::list<SyncedVictim>::const_iterator old_list_iter = old_local_synced_victims.begin(); old_list_iter != old_local_synced_victims.end(); old_list_iter++)
                {
                    if (list_iter->getKey() == old_list_iter->getVictimInfo().getKey())
                    {
                        tmp_dirinfo_set = old_list_iter->getDirinfoSet();
                        break;
                    }
                }

                map_iter->second.push_back(SyncedVictim(*list_iter, tmp_dirinfo_set)); // Insert new local synced victim
            }
        }

        return;
    }

    void VictimTracker::updateSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        checkPointers_();

        // Update directory info if the beaconed key is a local/neighbor synced victim
        // TODO: END HERE
    }

    void VictimTracker::checkPointers_() const
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        
        return;
    }
}