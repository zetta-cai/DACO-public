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

#include "core/victim/victim_info.h"
#include "core/victim_tracker.h"

namespace covered
{
    class CoveredCacheManager
    {
    public:
        CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt);
        ~CoveredCacheManager();

        void updateVictimTrackerForLocalSyncedVictimInfos(const std::list<VictimInfo>& local_synced_victim_infos);
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Non-const shared variables (each should be thread safe)

        // TODO: Aggregated uncached popularity for admission (thread safe)

        // Track per-edge-node least popular victims for placement and eviction (thread safe)
        VictimTracker victim_tracker_;
    };
}

#endif