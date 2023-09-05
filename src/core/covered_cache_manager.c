#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt) : popularity_aggregator_(edge_idx, edgecnt, popularity_aggregation_capacity_bytes, topk_edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    // For popularity aggregation

    void CoveredCacheManager::updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const bool& is_tracked_by_source_edge_node, const Popularity& local_uncached_popularity, const bool& is_global_cached)
    {
        popularity_aggregator_.updateAggregatedPopularity(key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, is_global_cached);
        return;
    }

    // For victim synchronization

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