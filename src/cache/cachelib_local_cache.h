/*
 * CachelibLocalCache: local edge cache with LRU2Q policy based on Cachelib (https://github.com/facebook/CacheLib).
 * 
 * NOTE: all configuration and function calls refer to Cachelib files, including lib/CacheLib/examples/simple_cache/main.cpp and lib/cachelib/cachebench/runner/CacheStressor.h.
 * 
 * NOTE: see notes on source code of cachelib in docs/cachelib.md.
 * 
 * By Siyuan Sheng (2023.08.07).
 */

#ifndef CACHELIB_LOCAL_CACHE_H
#define CACHELIB_LOCAL_CACHE_H

#include <string>

#include "cache/cachelib/CacheAllocator-inl.h"
#include "cache/local_cache_base.h"

namespace covered
{
    class CachelibLocalCache : public LocalCacheBase
    {
    public:
        typedef Lru2QAllocator CachelibLru2QCache; // LRU2Q cache policy
        typedef CachelibLru2QCache::Config Lru2QCacheConfig;
        typedef CachelibLru2QCache::ReadHandle Lru2QCacheReadHandle;
        typedef CachelibLru2QCache::Item Lru2QCacheItem;

        // NOTE: too small cache capacity cannot support slab-based memory allocation in cachelib (see lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h and lib/CacheLib/cachelib/allocator/memory/SlabAllocator.cpp)
        //static const uint64_t CACHELIB_MIN_CAPACITY_BYTES; // (OBSOLETE: move to Util) NOTE: NOT affect capacity constraint!

        CachelibLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~CachelibLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) override; // Return if key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful, const uint64_t& miss_latency_us = 0) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        virtual void invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) override; // Invoke some method-specific function for local edge cache
        virtual void invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const override; // Invoke some method-specific function for local edge cache

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const override;

        // Member variables

        // Const variable
        std::string instance_name_;
        uint32_t max_allocation_class_size_; // 4MiB in Cachelib by default

        // Non-const shared variables
        std::unique_ptr<CachelibLru2QCache> cachelib_cache_ptr_; // Data and metadata for local edge cache
        facebook::cachelib::PoolId cachelib_poolid_; // Pool ID for cachelib local edge cache
    };
}

#endif