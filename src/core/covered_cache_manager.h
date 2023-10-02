/*
 * CoveredCacheManager: COVERED's manager tracks metadata to integrate local edge cache and cooperation wrapper for cooperative-caching-aware cache management (thread safe).
 *
 * NOTE: in baselines with independent cache management, local edge cache (KV data and local cache metadata) and cooperation wrapper (content discovery and request redirection) are orthogonal/independent.
 * 
 * NOTE: in COVERED with cooperative-caching-aware cache management, local edge cache needs to provide/update metadata (e.g., local uncached popularity and current-edge-node victims), while cooperation wrapper needs to sync metadata (e.g., popularity aggregation and victim synchronization) to gain a global view of all involved edge nodes for cache management with cooperation awareness.
 * 
 * By Siyuan Sheng (2023.08.26).
 */

#ifndef COVERED_CACHE_MANAGER_H
#define COVERED_CACHE_MANAGER_H

#include <list>
#include <string>
#include <unordered_map>

#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "core/directory_cacher.h"
#include "core/popularity/edgeset.h"
#include "core/popularity_aggregator.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "core/victim_tracker.h"

namespace covered
{
    class CoveredCacheManager
    {
    public:
        CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& topk_edgecnt);
        ~CoveredCacheManager();

        // For selective popularity aggregation (may trigger trade-off-aware placement calculation)

        // NOTE: need_placement_calculation works only when key is tracked by local uncached metadata of sender edge node
        bool updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, const bool& need_placement_calculation, Edgeset& best_placement_edgeset); // Return if the best placement exists (i.e., with positive placement gain)
        void clearPopularityAggregatorForPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx);

        // For victim synchronization

        void updateVictimTrackerForLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets);
        void updateVictimTrackerForLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);

        VictimSyncset accessVictimTrackerForVictimSyncset() const;
        void updateVictimTrackerForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets);

        // For directory metadata cache

        bool accessDirectoryCacherForCachedDirectory(const Key& key, CachedDirectory& cached_directory) const; // Return if key is tracked by directory_cacher_
        bool accessDirectoryCacherToCheckPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const; // Return if key is tracked by directory_cacher_
        void updateDirectoryCacherToRemoveCachedDirectory(const Key& key);
        void updateDirectoryCacherForNewCachedDirectory(const Key& key, const CachedDirectory& cached_directory);

        uint64_t getSizeForCapacity() const;
    private:
        typedef DeltaReward PlacementGain; // Admission benefit - eviction cost for trade-off-aware cache placement and eviction

        static const std::string kClassName;

        // Perform placement calculation only if key belongs to a global popular uncached object (i.e., with large enough max admission benefit)
        // NOTE: best_placement_edgeset is used for perserved edgeset and placement notifications, while best_placement_peredge_victimset is used for victim removal (both for non-blocking placement deployment)
        bool placementCalculation_(const Key& key, const bool& is_global_cached, Edgeset& best_placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_victimset); // Return if the best placement exists (i.e., with positive placement gain)

        // Const shared variables
        std::string instance_name_;
        const uint32_t topk_edgecnt_; // Come from CLI

        // Non-const shared variables (each should be thread safe)
        // NOTE: we do NOT use per-key fine-grained locking here, as CoveredCacheManager does NOT need strong serializability/atomicity for its componenets, and some components (PopularityAggregator and VictimTracker) CANNOT use per-key fine-grained locking due to accessing multiple keys in one function (e.g., discardGlobalLessPopularObjects_() and updateLocalSyncedVictims())

        // Track aggregated uncached popularity for global admission (thread safe)
        PopularityAggregator popularity_aggregator_;

        // Track per-edge-node least popular victims for placement and eviction (thread safe)
        VictimTracker victim_tracker_;

        // Track cached directory of popular local uncached objects to reduce message overhead (thread safe)
        DirectoryCacher directory_cacher_;
    };
}

#endif