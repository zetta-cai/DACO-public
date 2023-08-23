/*
 * CoveredLocalCache: local edge cache with COVERED's policy built based on Cachelib (https://github.com/facebook/CacheLib.git).
 *
 * NOTE: To avoid hacking 352.6K LOC of CacheLib, we track metadata outside CacheLib; it is similar as MMTinyLFU in CacheLib, which maintains access frequencies outside MemoryAllocator (using a CMS in MM2QTinyLFU) due to NO need of slab-based memory management for metadata; specifically, we use CacheLib CacheItem to track key, value, LRU, and lookup information (cache size usage of this part has already been counted by CacheLib), yet maintain other metadata and sorted popularity outside CacheLib in CoveredLocalCache.
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
#include "cache/covered/local_cache_metadata.h"
#include "cache/local_cache_base.h"

// NOTE: although we track local cached metadata outside CacheLib to avoid extensive hacking, per-key metadata and popularity iterator actually can be stored into cachelib::CacheItem -> so we do NOT need to count the size of keys for local cached metadata (similar as size measurement in ValidityMap and BlockTracker)
// NOTE: we still count the size of keys for local uncached metadata -> but only once for per-key metadata and popularity iterator, as they actually can be maintained in a single map (we just split them for implementation simplicity)

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

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related) const override; // Triggered by get/put/delrsp for cache miss for admission policy if any

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

        // Update local cached metadata
        //void updateLocalCachedStatisticsForAdmission_(const Key& key, const Value& value) const; // Triggered by admission of newly admitted objects
        //void updateLocalCachedStatisticsForCachedKeyValue_(const Key& key, const Value& value) const; // Triggered by put requests with cache hits
        //void updateLocalCachedStatisticsForEviction_(const Key& key) const; // Triggered by eviction of victim objects

        // Update local uncached metadata
        //void updateLocalUncachedStatisticsForNewlyTracked_(const Key& key, const Value& value) const;// Triggered by newly tracked objects in candidate list
        //void updateLocalUncachedStatisticsForCandidateKeyValue_(const Key& key) const; // Triggered by put/del responses for cache misses
        //void updateLocalUncachedStatisticsForDetracked_(const Key& key) const; // Triggered by detracked objects in candidate list

        // Member variables

        // (A) Const variable
        std::string instance_name_;

        // (B) Non-const shared variables of local cached objects for eviction

        // CacheLib-based key-value storage
        std::unique_ptr<LruCache> covered_cache_ptr_; // Data engine for local edge cache
        facebook::cachelib::PoolId covered_poolid_; // Pool ID for covered local edge cache

        mutable LocalCacheMetadata local_cached_metadata_; // Metadata for local cached objects

        // (C) Non-const shared variables of local uncached objects for admission

        mutable LocalCacheMetadata local_uncached_metadata_; // Metadata for local uncached objects
    };
}

#endif