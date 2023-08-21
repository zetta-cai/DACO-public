/*
 * CoveredLocalCache: local edge cache with COVERED's policy built based on Cachelib (https://github.com/facebook/CacheLib.git).
 *
 * NOTE: To avoid hacking 352.6K LOC of CacheLib, we track statistics outside CacheLib; it is similar as MMTinyLFU in CacheLib, which maintains access frequencies outside MemoryAllocator (using a CMS in MM2QTinyLFU) due to NO need of slab-based memory management for statistics; specifically, we use CacheLib CacheItem to track key, value, LRU, and lookup information (cache size usage of this part has already been counted by CacheLib), yet maintain other statistics and sorted popularity outside CacheLib in CoveredLocalCache.
 * 
 * See other NOTEs in src/cache/cachelib_local_cache.h
 * 
 * By Siyuan Sheng (2023.08.15).
 */

#ifndef COVERED_LOCAL_CACHE_H
#define COVERED_LOCAL_CACHE_H

#include <list> // std::list
#include <string>

#include "cache/covered/common_header.h"
#include "cache/covered/perkey_statistics_map.h"
#include "cache/covered/perkey_statistics_list.h"
#include "cache/covered/pergroup_statistics_map.h"
#include "cache/covered/sorted_popularity_multimap.h"
#include "cache/local_cache_base.h"

// NOTE: although we track local cached statistics outside CacheLib to avoid extensive hacking, per-key statistics and popularity iterator actually can be stored into cachelib::CacheItem -> so we do NOT need to count the size of keys for local cached statistics (similar as size measurement in ValidityMap and BlockTracker)
// NOTE: we still count the size of keys for local uncached statistics -> but only once for per-key statistics and popularity iterator, as they actually can be maintained in a single map (we just split them for implementation simplicity)

// TODO: Use homogeneous popularity calculation now, but will replace with heterogeneous popularity calculation + learning later (for both local cached and uncached objects)
// TODO: Use learned index to replace local cached/uncached sorted_popularity_ for less memory usage (especially for local cached objects due to limited # of uncached objects)

// TODO: Use grouping settings in GL-Cache later (e.g., fix per-group cache size usage instead of COVERED_PERGROUP_MAXKEYCNT)

namespace covered
{
    class CoveredLocalCache : public LocalCacheBase
    {
    public:
        // NOTE: too small cache capacity cannot support slab-based memory allocation in cachelib (see lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h and lib/CacheLib/cachelib/allocator/memory/SlabAllocator.cpp)
        static const uint64_t COVERED_MIN_CAPACITY_BYTES; // NOTE: NOT affect capacity constraint!

        CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~CoveredLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const override;
        virtual bool evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value) override;
        virtual void evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;

        // (5) COVERED-specific functions

        // Update local cached statistics
        //void updateLStatisticsForAdmission_(const Key& key); // Triggered by admission of newly admitted objects
        void updateStatisticsForCachedKey_(const Key& key); // Triggered by get requests with cache hits
        //void updateStatisticsForCachedKeyValue_(const Key& key); // Triggered by put requests with cache hits
        //void updateStatisticsForEviction_(const Key& key); // Triggered by eviction of victim objects

        // Update local uncached statistics
        //void updateStatisticsForNewlyTracked_(const Key& key);// Triggered by newly tracked objects in candidate list
        //void updateStatisticsForCandidateKey_(const Key& key); // Triggered by get responses for cache misses
        //void updateStatisticsForCandidateKeyValue_(const Key& key); // Triggered by get responses for cache misses
        //void updateStatisticsForDetracked_(const Key& key); // Triggered by detracked objects in candidate list

        // Member variables

        // (A) Const variable
        std::string instance_name_;

        // (B) Non-const shared variables of local cached objects for eviction

        // CacheLib-based key-value storage
        std::unique_ptr<LruCache> covered_cache_ptr_; // Data and metadata for local edge cache (including key, value, LRU list, and lookup hashtable)
        facebook::cachelib::PoolId covered_poolid_; // Pool ID for covered local edge cache

        PerkeyStatisticsMap local_cached_perkey_statistics_map_; // Local cached object-level statistics (NOT include recency which has been tracked by covered_cache_ptr_)
        PergroupStatisticsMap local_cached_pergroup_statistics_map_; // Local cached group-level statistics (grouping based on admission time)
        SortedPopularityMultimap local_cached_sorted_popularity_; // Local cached sorted popularity information (ascending order; allow duplicate popularity values)

        // (C) Non-const shared variables of local uncached objects for admission

        PerkeyStatisticsList local_uncached_perkey_statistics_candidate_list_; // LRU-based candidate list for limited uncached objects; local uncached object-level statistics (NOT include recency which is tracked by list index)
        PergroupStatisticsMap local_uncached_pergroup_statistics_map_; // Local uncached group-level statistics (grouping based on tracked time)
        SortedPopularityMultimap local_uncached_sorted_popularity_; // Local uncached sorted popularity information (ascending order; allow duplicate popularity values)
    };
}

#endif