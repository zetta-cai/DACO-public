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

#include <string>

#include "cache/cachelib/CacheAllocator-inl.h"
#include "cache/local_cache_base.h"

namespace covered
{
    class CoveredLocalCache : public LocalCacheBase
    {
    public:
        typedef LruAllocator LruCache; // LRU2Q cache policy
        typedef LruCache::Config LruCacheConfig;
        typedef LruCache::ReadHandle LruCacheReadHandle;
        typedef LruCache::Item LruCacheItem;

        typedef uint32_t CoveredPerkeyStatistics;
        typedef uint32_t CoveredGroupId;
        typedef uint32_t CoveredPergroupStatistics;

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

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        std::unique_ptr<LruCache> covered_cache_ptr_; // Data and metadata for local edge cache (including key, value, LRU list, and lookup hashtable)
        facebook::cachelib::PoolId covered_poolid_; // Pool ID for covered local edge cache
        // NOTE: although we track object-level statistics outside CacheLib to avoid extensive hacking, it actually can be stored into cachelib::CacheItem -> so we do NOT need to count the size of keys for object-level statistics here (similar as size measurement in ValidityMap and BlockTracker)
        std::unordered_map<Key, CoveredPerkeyStatistics, KeyHasher> perkey_statistics_; // Object-level statistics (NOT include recency which has been tracked by covered_cache_ptr_)
        // NOTE: we still count all cache size usage for group-level statistics and sorted popularity information!
        std::unordered_map<CoveredGroupId, CoveredPergroupStatistics> pergroup_statistics_; // Group-level statistics
        std::multimap<std::uint32_t, LruCacheReadHandle> sorted_popularity_; // Sorted popularity information (ascending order; allow duplicate popularity values)
    };
}

#endif