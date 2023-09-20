#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& topk_edgecnt) : popularity_aggregator_(edge_idx, edgecnt, popularity_aggregation_capacity_bytes, topk_edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt), directory_cacher_(edge_idx, popularity_collection_change_ratio)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    // For popularity aggregation

    void CoveredCacheManager::updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& need_placement_calculation)
    {
        popularity_aggregator_.updateAggregatedUncachedPopularity(key, source_edge_idx, collected_popularity, is_global_cached);
        
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid after cache placement
        if (need_placement_calculation)
        {
            const bool is_tracked_by_source_edge_node = collected_popularity.isTracked();
            // NOTE: NO need to perform placement calculation if key is NOT tracked by source edge node, as removing old local uncached popularity if any will NEVER increase admission benefit
            if (is_tracked_by_source_edge_node)
            {
                // Perform greedy-based placement calculation for trade-off-aware cache placement
                placementCalculation_(key, is_global_cached);
            }
        }

        return;
    }

    void CoveredCacheManager::clearPopularityAggregatorAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        popularity_aggregator_.clearAggregatedUncachedPopularityAfterAdmission(key, source_edge_idx);
        return;
    }

    // For victim synchronization

    void CoveredCacheManager::updateVictimTrackerForLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets)
    {
        victim_tracker_.updateLocalSyncedVictims(local_cache_margin_bytes, local_synced_victim_cacheinfos, local_beaconed_local_synced_victim_dirinfosets);
        return;
    }

    void CoveredCacheManager::updateVictimTrackerForLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        victim_tracker_.updateLocalBeaconedVictimDirinfo(key, is_admit, directory_info);
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

    bool CoveredCacheManager::accessDirectoryCacherForCachedDirectory(const Key& key, CachedDirectory& cached_directory) const
    {
        bool has_cached_directory = directory_cacher_.getCachedDirectory(key, cached_directory);
        return has_cached_directory;
    }

    bool CoveredCacheManager::accessDirectoryCacherToCheckPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const
    {
        bool has_cached_directory = directory_cacher_.checkPopularityChange(key, local_uncached_popularity, cached_directory, is_large_popularity_change);
        return has_cached_directory;
    }

    void CoveredCacheManager::updateDirectoryCacherToRemoveCachedDirectory(const Key& key)
    {
        directory_cacher_.removeCachedDirectoryIfAny(key);
        return;
    }

    void CoveredCacheManager::updateDirectoryCacherForNewCachedDirectory(const Key& key, const CachedDirectory& cached_directory)
    {
        directory_cacher_.updateForNewCachedDirectory(key, cached_directory);
        return;
    }

    uint64_t CoveredCacheManager::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += victim_tracker_.getSizeForCapacity();

        return total_size;
    }

    void CoveredCacheManager::placementCalculation_(const Key& key, const bool& is_global_cached)
    {
        AggregatedUncachedPopularity tmp_aggregated_uncached_popularity;
        bool has_aggregated_uncached_popularity = popularity_aggregator_.getAggregatedUncachedPopularity(key, tmp_aggregated_uncached_popularity);

        // Perform placement calculation ONLY if key is still tracked by popularity aggregator (i.e., belonging to a global popular uncached object)
        if (has_aggregated_uncached_popularity)
        {
            const ObjectSize tmp_object_size = tmp_aggregated_uncached_popularity.getObjectSize();
            const uint32_t tmp_topk_list_length = tmp_aggregated_uncached_popularity.getTopkListLength();
            assert(tmp_topk_list_length > 0); // NOTE: we perform placement calculation only when add/update a new local uncached popularity -> at least one local uncached popularity in the top-k list
            assert(tmp_topk_list_length <= popularity_aggregator_.getTopkEdgecnt()); // At most EdgeCLI::covered_topk_edgecnt_ times

            // Greedy-based placement calculation
            std::vector<PlacementGain> placement_gains;
            for (uint32_t topicnt = 1; topicnt <= tmp_topk_list_length; topicnt++)
            {
                // Consider topi edge nodes ordered by local uncached popularity in a descending order
                std::unordered_set<uint32_t> tmp_placement_edgeset;
                const DeltaReward tmp_admission_benefit = tmp_aggregated_uncached_popularity.calcAdmissionBenefit(topicnt, is_global_cached, tmp_placement_edgeset);

                // END HERE
                // TODO: Calculate eviction cost based on tmp_placement_edgeset
            }
        }

        return;
    }
}