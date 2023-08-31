#include "core/victim_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

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

        size_bytes_ = 0;
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

        // Acquire a write lock to update local synced victims atomically
        std::string context_name = "VictimTracker::updateLocalSyncedVictims()";
        rwlock_for_victim_tracker_->acquire_lock(context_name);

        // NOTE: limited computation overhead to update local synced victim infos, as we track limited number of local synced victims for the current edge node

        // Update cacheinfos of local synced victims for the current edge node
        std::list<VictimCacheinfo> old_local_synced_victim_cacheinfos;
        peredge_victim_cacheinfos_t::iterator cacheinfo_map_iter = peredge_victim_cacheinfos_.find(edge_idx_);
        if (cacheinfo_map_iter == peredge_victim_cacheinfos_.end()) // No local synced victims for the current edge node
        {
            // Add latest local synced victims for the current edge node
            cacheinfo_map_iter = peredge_victim_cacheinfos_.insert(std::pair(edge_idx_, local_synced_victim_cacheinfos)).first;
            assert(cacheinfo_map_iter != peredge_victim_cacheinfos_.end());

            size_bytes_ = Util::uint64Add(size_bytes_, sizeof(uint32_t)); // For edge_idx_
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
            size_bytes_ = Util::uint64Add(size_bytes_, new_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each latest local synced victims

            const Key& tmp_key = new_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            if (dirinfo_map_iter == perkey_victim_dirinfo_.end()) // No victim dirinfo for the key
            {
                // Add a new victim dirinfo for the key
                dirinfo_map_iter = perkey_victim_dirinfo_.insert(std::pair(tmp_key, VictimDirinfo())).first;
                assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

                dirinfo_map_iter->second.incrRefcnt();

                size_bytes_ = Util::uint64Add(size_bytes_, (tmp_key.getKeystr().length() + dirinfo_map_iter->second.getSizeForCapacity())); // For each inserted victim dirinfo of a new local synced victim
            }
            else // The key already has a victim dirinfo
            {
                dirinfo_map_iter->second.incrRefcnt();
            }
        }

        // Remove victim dirinfos for old local synced victim keys
        for (std::list<VictimCacheinfo>::const_iterator old_cacheinfo_list_iter = old_local_synced_victim_cacheinfos.begin(); old_cacheinfo_list_iter != old_local_synced_victim_cacheinfos.end(); ++old_cacheinfo_list_iter)
        {
            size_bytes_ = Util::uint64Minus(size_bytes_, old_cacheinfo_list_iter->getSizeForCapacity()); // For cacheinfo of each old local synced victim

            const Key& tmp_key = old_cacheinfo_list_iter->getKey();
            perkey_victim_dirinfo_t::iterator dirinfo_map_iter = perkey_victim_dirinfo_.find(tmp_key);
            assert(dirinfo_map_iter != perkey_victim_dirinfo_.end());

            dirinfo_map_iter->second.decrRefcnt();
            if (dirinfo_map_iter->second.getRefcnt() == 0) // No edge node is tracking the key as a synced victim
            {
                size_bytes_ = Util::uint64Minus(size_bytes_, (tmp_key.getKeystr().length() + dirinfo_map_iter->second.getSizeForCapacity())); // For each erased victim dirinfo of a old local synced victim

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

        rwlock_for_victim_tracker_->unlock(context_name);

        return;
    }

    void VictimTracker::updateSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        checkPointers_();

        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "VictimTracker::updateSyncedVictimDirinfo()";
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

    void VictimTracker::checkPointers_() const
    {
        assert(rwlock_for_victim_tracker_ != NULL);
        
        return;
    }
}