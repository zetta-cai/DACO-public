#include "core/covered_cache_manager.h"

#include <sstream>

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& topk_edgecnt) : topk_edgecnt_(topk_edgecnt), popularity_aggregator_(edge_idx, edgecnt, popularity_aggregation_capacity_bytes, topk_edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt), directory_cacher_(edge_idx, popularity_collection_change_ratio)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    // For popularity aggregation

    bool CoveredCacheManager::updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, const bool& need_placement_calculation, Edgeset& best_placement_edgeset)
    {
        // Double-check preserved edgeset to set is global cached flag to avoid over-estimating (max) admission benefit
        bool tmp_is_global_cached = is_global_cached;
        if (!is_global_cached)
        {
            // Set is_global_cached to true if key is being admitted
            bool is_being_admitted = popularity_aggregator_.isKeyBeingAdmitted(key);
            if (is_being_admitted)
            {
                tmp_is_global_cached = true;
            }
        }

        popularity_aggregator_.updateAggregatedUncachedPopularity(key, source_edge_idx, collected_popularity, tmp_is_global_cached, is_source_cached);
        
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid after cache placement
        bool has_best_placement = false;
        if (need_placement_calculation)
        {
            const bool is_tracked_by_source_edge_node = collected_popularity.isTracked();
            // NOTE: NO need to perform placement calculation if key is NOT tracked by source edge node, as removing old local uncached popularity if any will NEVER increase admission benefit
            if (is_tracked_by_source_edge_node)
            {
                // Perform greedy-based placement calculation for trade-off-aware cache placement
                // NOTE: set best_placement_edgeset for preserved edgeset and placement notifications; set best_placement_peredge_victimset for victim removal to avoid duplicate eviction (both for non-blocking placement deployment)
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> best_placement_peredge_victimset;
                has_best_placement = placementCalculation_(key, tmp_is_global_cached, best_placement_edgeset, best_placement_peredge_victimset);
                assert(best_placement_edgeset.size() <= topk_edgecnt_); // At most k placement edge nodes each time

                if (has_best_placement)
                {
                    // Preserve placement edgeset + perform local-uncached-popularity removal to avoid duplicate placement, and update max admission benefit with aggregated uncached popularity for non-blocking placement deployment
                    popularity_aggregator_.updatePreservedEdgesetForPlacement(key, best_placement_edgeset, tmp_is_global_cached);

                    // Remove involved victims from victim tracker for each edge node in placement edgeset to avoid duplication eviction
                    // NOTE: removed victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node
                    for (std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>::const_iterator best_placement_peredge_victimset_const_iter = best_placement_peredge_victimset.begin(); best_placement_peredge_victimset_const_iter != best_placement_peredge_victimset.end(); best_placement_peredge_victimset_const_iter++)
                    {
                        const uint32_t tmp_edge_idx = best_placement_peredge_victimset_const_iter->first;
                        const std::unordered_set<Key, KeyHasher>& tmp_victim_keyset_const_ref = best_placement_peredge_victimset_const_iter->second;
                        victim_tracker_.removeVictimsForGivenEdge(tmp_edge_idx, tmp_victim_keyset_const_ref);
                    }
                }
            }
        }

        return has_best_placement;
    }

    void CoveredCacheManager::clearPopularityAggregatorForPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        popularity_aggregator_.clearPreservedEdgesetAfterAdmission(key, source_edge_idx);

        // NOTE: all old local uncached popularities have already been cleared right after placement calculation in updatePreservedEdgesetForPlacement()
        // (1) NO need to clear old local uncached popularity for the source edge node
        //updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, false, 0.0, 0, true);
        // (2) Assert old local uncached popularity for the source edge node MUST NOT exist
        assertNoLocalUncachedPopularity(key, source_edge_idx);

        return;
    }

    void CoveredCacheManager::assertNoLocalUncachedPopularity(const Key& key, const uint32_t& source_edge_idx) const
    {
        // Assert old local uncached popularity for the source edge node MUST NOT exist
        AggregatedUncachedPopularity tmp_aggregated_uncached_popularity;
        bool has_aggregated_uncached_popularity = popularity_aggregator_.getAggregatedUncachedPopularity(key, tmp_aggregated_uncached_popularity);
        if (has_aggregated_uncached_popularity)
        {
            assert(tmp_aggregated_uncached_popularity.hasLocalUncachedPopularity(source_edge_idx) == false);
        }
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

    bool CoveredCacheManager::placementCalculation_(const Key& key, const bool& is_global_cached, Edgeset& best_placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_victimset)
    {
        bool has_best_placement = false;
        
        // For lazy victim fetching before non-blocking placement deployment
        bool need_victim_fetching = false;
        Edgeset best_placement_victim_fetch_edgeset;

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
            PlacementGain max_placement_gain = 0.0;
            uint32_t best_topicnt = 0;
            Edgeset tmp_best_placement_edgeset; // For preserved edgeset and placement notifications under non-blocking placement deployment
            std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_best_placement_peredge_victimset; // For victim removal under non-blocking placement deployment
            for (uint32_t topicnt = 1; topicnt <= tmp_topk_list_length; topicnt++)
            {
                // Consider topi edge nodes ordered by local uncached popularity in a descending order

                // Calculate admission benefit if we place the object with is_global_cached flag into topi edge nodes
                Edgeset tmp_placement_edgeset;
                const DeltaReward tmp_admission_benefit = tmp_aggregated_uncached_popularity.calcAdmissionBenefit(topicnt, is_global_cached, tmp_placement_edgeset);

                // Calculate eviction cost based on tmp_placement_edgeset
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_placement_peredge_victimset;
                Edgeset tmp_best_placement_victim_fetch_edgeset;
                const DeltaReward tmp_eviction_cost = victim_tracker_.calcEvictionCost(tmp_object_size, tmp_placement_edgeset, tmp_placement_peredge_victimset, tmp_best_placement_victim_fetch_edgeset); // NOTE: tmp_eviction_cost may be partial eviction cost if without enough victims

                // Calculate placement gain (admission benefit - eviction cost)
                const DeltaReward tmp_placement_gain = tmp_admission_benefit - tmp_eviction_cost;
                if ((topicnt == 1) || (tmp_placement_gain > max_placement_gain))
                {
                    max_placement_gain = tmp_placement_gain;
                    best_topicnt = topicnt;
                    tmp_best_placement_edgeset = tmp_placement_edgeset;
                    tmp_best_placement_peredge_victimset = tmp_placement_peredge_victimset;
                    best_placement_victim_fetch_edgeset = tmp_best_placement_victim_fetch_edgeset;
                }
            }

            // Update best placement if any
            if (max_placement_gain > 0.0)
            {
                if (best_placement_victim_fetch_edgeset.size() == 0) // NO need for lazy victim fetching
                {
                    has_best_placement = true;
                    best_placement_edgeset = tmp_best_placement_edgeset;
                    best_placement_peredge_victimset = tmp_best_placement_peredge_victimset;
                }
                else // Need lazy victim fetching
                {
                    // Fetch victims ONLY if admission benefit > partial eviction cost under the best placement
                    need_victim_fetching = true;
                }
            }
        }

        // Lazy victim fetching before non-blocking placement deployment (minor cases ONLY if cache margin bytes + size of victims < object size and admission benefit > partial eviction cost; a small per-edge victim cnt is sufficient for most admissions)
        // TODO: Maintain a small vicitm cache in each beacon edge node if with frequent lazy victim fetching to avoid degrading directory lookup performance
        if (need_victim_fetching)
        {
            // TODO: Issue CoveredVictimFetchRequest to fetch more victims (note that CoveredVictimFetchRequest is a foreground message before non-blocking placement deployment)
            // TODO: Redo placement calculation after fetching more victims
        }

        return has_best_placement;
    }
}