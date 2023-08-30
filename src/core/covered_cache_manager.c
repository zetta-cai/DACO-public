#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt) : victim_tracker_(edge_idx, peredge_synced_victimcnt)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    void CoveredCacheManager::updateVictimTrackerForLocalSyncedVictimInfos(const std::list<VictimInfo>& local_synced_victim_infos)
    {
        victim_tracker_.updateLocalSyncedVictimInfos(local_synced_victim_infos);
        return;
    }

    void CoveredCacheManager::updateVictimTrackerForSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        victim_tracker_.updateSyncedVictimDirinfo(key, is_admit, directory_info);
        return;
    }
}