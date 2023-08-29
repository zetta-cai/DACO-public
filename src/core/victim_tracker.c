#include "core/victim_tracker.h"

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

        peredge_synced_victims_.clear();
    }

    VictimTracker::~VictimTracker()
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        delete rwlock_for_victim_tracker_;
        rwlock_for_victim_tracker_ = NULL;
    }

    void VictimTracker::updateLocalSyncedVictimInfos(const std::list<VictimInfo>& local_synced_victim_infos)
    {
        checkPointers_();

        assert(local_synced_victim_infos.size() <= peredge_synced_victimcnt_);

        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        // Check synced victims of current edge node
        peredge_synced_victims_t::iterator map_iter = peredge_synced_victims_.find(edge_idx_);
        if (map_iter == peredge_synced_victims_.end()) // No local synced victims for the current edge node
        {
            map_iter = peredge_synced_victims_.insert(std::pair(edge_idx_, std::list<SyncedVictim>())).first;
            assert(map_iter != peredge_synced_victims_.end());

            for (std::list<VictimInfo>::const_iterator list_iter = local_synced_victim_infos.begin(); list_iter != local_synced_victim_infos.end(); list_iter++)
            {
                map_iter->second.push_back(SyncedVictim(*list_iter, DirectoryInfo())); // Use empty DirectoryInfo for newly inserted SyncedVictim
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
                DirectoryInfo tmp_dirinfo;
                for (std::list<SyncedVictim>::const_iterator old_list_iter = old_local_synced_victims.begin(); old_list_iter != old_local_synced_victims.end(); old_list_iter++)
                {
                    if (list_iter->getKey() == old_list_iter->getVictimInfo().getKey())
                    {
                        tmp_dirinfo = old_list_iter->getDirinfo();
                        break;
                    }
                }

                map_iter->second.push_back(SyncedVictim(*list_iter, tmp_dirinfo)); // Insert new local synced victim
            }
        }

        return;
    }

    void VictimTracker::checkPointers_() const
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        
        return;
    }
}