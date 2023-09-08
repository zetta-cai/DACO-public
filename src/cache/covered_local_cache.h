/*
 * CoveredLocalCache: local edge cache with COVERED's policy built based on Cachelib (https://github.com/facebook/CacheLib.git).
 *
 * NOTE: To avoid hacking 352.6K LOC of CacheLib, we track metadata outside CacheLib; it is similar as MMTinyLFU in CacheLib, which maintains access frequencies outside MemoryAllocator (using a CMS in MM2QTinyLFU) due to NO need of slab-based memory management for metadata; specifically, we use CacheLib CacheItem to track key, value, LRU, and lookup information (cache size usage of this part has already been counted by CacheLib), yet maintain other metadata and sorted popularity outside CacheLib in CoveredLocalCache (see other NOTEs in src/cache/cachelib_local_cache.h).
 * 
 * NOTE: we track local cached metadata and calculate local rewards to find local synced victims for victim synchronization; we track local uncached metadata and calculate local admission benefits for popularity collection.
 * 
 * By Siyuan Sheng (2023.08.15).
 */

#ifndef COVERED_LOCAL_CACHE_H
#define COVERED_LOCAL_CACHE_H

#include <list> // std::list
#include <string>

#include "cache/covered/common_header.h"
#include "cache/covered/local_cached_metadata.h"
#include "cache/covered/local_uncached_metadata.h"
#include "cache/local_cache_base.h"

namespace covered
{
    class CoveredLocalCache : public LocalCacheBase
    {
    public:
        // NOTE: too small cache capacity cannot support slab-based memory allocation in cachelib (see lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h and lib/CacheLib/cachelib/allocator/memory/SlabAllocator.cpp)
        static const uint64_t COVERED_MIN_CAPACITY_BYTES; // NOTE: NOT affect capacity constraint!

        CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt);
        virtual ~CoveredLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const override;
        virtual std::list<VictimCacheinfo> getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const override; // Return up to peredge_synced_victimcnt local synced victims with the least local rewards
        virtual bool getLocalUncachedPopularityFromLocalCacheInternal_(const Key& key, Popularity& local_uncached_popularity) const override; // Return true if local uncached key is tracked (for piggybacking-based popularity colleciton)

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker) override;

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const override; // Triggered by get/put/delrsp for cache miss for admission policy if any

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;

        // Member variables

        // (A) Const variable
        std::string instance_name_;
        const uint32_t peredge_synced_victimcnt_;

        // (B) Non-const shared variables of local cached objects for eviction

        // CacheLib-based key-value storage
        std::unique_ptr<CachelibLruCache> covered_cache_ptr_; // Data engine for local edge cache
        facebook::cachelib::PoolId covered_poolid_; // Pool ID for covered local edge cache

        mutable LocalCachedMetadata local_cached_metadata_; // Metadata for local cached objects

        // (C) Non-const shared variables of local uncached objects for admission

        mutable LocalUncachedMetadata local_uncached_metadata_; // Metadata for local uncached objects
    };
}

#endif