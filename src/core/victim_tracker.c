#include "core/victim_tracker.h"

#include <assert.h>
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

        peredge_victim_cacheinfos_.clear();
        perkey_victim_dirinfo_.clear();
    }

    VictimTracker::~VictimTracker()
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        delete rwlock_for_victim_tracker_;
        rwlock_for_victim_tracker_ = NULL;
    }

    void VictimTracker::updateLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& beaconed_local_synced_victim_dirinfosets)
    {
        checkPointers_();

        assert(local_synced_victim_cacheinfos.size() <= peredge_synced_victimcnt_);
        assert(beaconed_local_synced_victim_dirinfosets.size() <= local_synced_victim_cacheinfos.size());

        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        // Update cacheinfos of local synced victims for the current edge node
        std::list<VictimCacheinfo> old_local_synced_victim_cacheinfos;
        peredge_victim_cacheinfos_t::iterator cacheinfo_map_iter = peredge_victim_cacheinfos_.find(edge_idx_);
        if (cacheinfo_map_iter == peredge_victim_cacheinfos_.end()) // No local synced victims for the current edge node
        {
            // Add latest local synced victims for the current edge node
            cacheinfo_map_iter = peredge_victim_cacheinfos_.insert(std::pair(edge_idx_, local_synced_victim_cacheinfos)).first;
            assert(cacheinfo_map_iter != peredge_victim_cacheinfos_.end());
        }
        else // Current edge node has tracked some local synced victims before
        {
            // Preserve old local synced victim cacheinfos if any
            old_local_synced_victim_cacheinfos = cacheinfo_map_iter->second;

            // Replace old local synced victim cacheinfos with the latest ones
            cacheinfo_map_iter->second = local_synced_victim_cacheinfos;
        }

        // Update victim dirinfos for new local synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator new_cacheinfo_list_iter = local_synced_victim_cacheinfos.begin(); new_cacheinfo_list_iter != local_synced_victim_cacheinfos.end(); ++new_cacheinfo_list_iter)
        {
            const Key& tmp_key = new_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            if (dirinfo_map_iter == perkey_victim_dirinfo_.end()) // No victim dirinfo for the key
            {
                // Add a new victim dirinfo for the key
                dirinfo_map_iter = perkey_victim_dirinfo_.insert(std::pair(tmp_key, VictimDirinfo())).first;
                assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

                dirinfo_map_iter->second.incrRefcnt();
            }
            else // The key already has a victim dirinfo
            {
                dirinfo_map_iter->second.incrRefcnt();
            }
        }

        // Remove victim dirinfos for old local synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator old_cacheinfo_list_iter = old_local_synced_victim_cacheinfos.begin(); old_cacheinfo_list_iter != old_local_synced_victim_cacheinfos.end(); ++old_cacheinfo_list_iter)
        {
            const Key& tmp_key = old_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

            dirinfo_map_iter->second.decrRefcnt();
            if (dirinfo_map_iter->second.getRefcnt() == 0) // No edge node is tracking the key as a synced victim
            {
                // Remove the victim dirinfo for the key
                perkey_victim_dirinfo_.erase(dirinfo_map_iter);
            }
        }

        // Update victim dirinfos for local beaconed victims
        for (std::unordered_map<Key, dirinfo_set_t, KeyHasher>::const_iterator beaconed_dirinfosets_map_iter = beaconed_local_synced_victim_dirinfosets.begin(); beaconed_dirinfosets_map_iter != beaconed_local_synced_victim_dirinfosets.end(); ++beaconed_dirinfosets_map_iter)
        {
            const Key& tmp_key = beaconed_dirinfosets_map_iter->first;
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

            dirinfo_map_iter->second.markLocalBeaconed();
            dirinfo_map_iter->second.setDirinfoSet(beaconed_dirinfosets_map_iter->second);
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