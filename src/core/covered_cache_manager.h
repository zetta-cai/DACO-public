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
#include "core/popularity/popularity_aggregator.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "core/victim_tracker.h"

namespace covered
{
    class CoveredCacheManager
    {
    public:
        CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt);
        ~CoveredCacheManager();

        // For selective popularity aggregation

        void updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached);
        void clearPopularityAggregatorAfterAdmission(const Key& key, const uint32_t& source_edge_idx);
        
        // For victim synchronization

        void updateVictimTrackerForLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets);
        void updateVictimTrackerForSyncedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);

        VictimSyncset accessVictimTrackerForVictimSyncset() const;
        void updateVictimTrackerForVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& victim_syncset, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets);

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Non-const shared variables (each should be thread safe)

        // Track aggregated uncached popularity for global admission (thread safe)
        PopularityAggregator popularity_aggregator_;

        // Track per-edge-node least popular victims for placement and eviction (thread safe)
        VictimTracker victim_tracker_;
    };
}

#endif