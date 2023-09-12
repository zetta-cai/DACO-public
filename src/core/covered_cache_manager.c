#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt) : popularity_aggregator_(edge_idx, edgecnt, popularity_aggregation_capacity_bytes, topk_edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt), directory_cacher_(edge_idx)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    // For popularity aggregation

    void CoveredCacheManager::updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached)
    {
        popularity_aggregator_.updateAggregatedUncachedPopularity(key, source_edge_idx, collected_popularity, is_global_cached);
        return;
    }

    void CoveredCacheManager::clearPopularityAggregatorAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        popularity_aggregator_.clearAggregatedUncachedPopularityAfterAdmission(key, source_edge_idx);
        return;
    }

    // For victim synchronization

    void CoveredCacheManager::updateVictimTrackerForLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets)
    {
        victim_tracker_.updateLocalSyncedVictims(local_synced_victim_cacheinfos, local_beaconed_local_synced_victim_dirinfosets);
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

    void CoveredCacheManager::updateVictimTrackerForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets)
    {
        victim_tracker_.updateForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);
        return;
    }

    // For directory metadata cache

    bool CoveredCacheManager::accessDirectoryCacherForCachedDirinfo(const Key& key, DirectoryInfo& dirinfo) const
    {
        bool has_cached_dirinfo = directory_cacher_.getCachedDirinfo(key, dirinfo);
        return has_cached_dirinfo;
    }

    uint64_t CoveredCacheManager::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += victim_tracker_.getSizeForCapacity();

        return total_size;
    }
}