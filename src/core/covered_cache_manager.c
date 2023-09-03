#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt) : popularity_aggregator_(edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    void CoveredCacheManager::updateVictimTrackerForLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& beaconed_local_synced_victim_dirinfosets)
    {
        victim_tracker_.updateLocalSyncedVictims(local_synced_victim_cacheinfos, beaconed_local_synced_victim_dirinfosets);
        return;
    }

    void CoveredCacheManager::updateVictimTrackerForSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        victim_tracker_.updateSyncedVictimDirinfo(key, is_admit, directory_info);
        return;
    }

    VictimSyncset CoveredCacheManager::accessVictimTrackerForVictimSyncset() const
    {
        VictimSyncset victim_syncset = victim_tracker_.getVictimSyncset();
        return victim_syncset;
    }

    uint64_t CoveredCacheManager::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += victim_tracker_.getSizeForCapacity();

        return total_size;
    }
}