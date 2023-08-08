/*
 * CachelibLocalCache: local edge cache with LRU2Q policy based on Cachelibhttps://github.com/facebook/CacheLib.git) (thread safe).
 *
 * NOTE: all non-const shared variables in CachelibLocalCache should be thread safe.
 * 
 * NOTE: all configuration and function calls refer to Cachelib files, including lib/cachelib/examples/simple_cache/main.cpp and lib/cachelib/cachebench/runner/CacheStressor.h.
 * 
 * NOTE: handle points to CacheItem, whose kAllocation alloc_ stores key and value (it encodes key size and value size into one uint32_t variable, and concatenates key bytes and value bytes into one unsigned char array) -> getMemory() and getSize() return value bytes and size, while getKey() return Key (inheriting from folly::StringPiece) with key bytes and size.
 * 
 * By Siyuan Sheng (2023.08.07).
 */

#ifndef CACHELIB_LOCAL_CACHE_H
#define CACHELIB_LOCAL_CACHE_H

#include <string>

#include "cache/cachelib/CacheAllocator.h"
#include "cache/local_cache_base.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class CachelibLocalCache : public LocalCacheBase
    {
    public:
        typedef Lru2QAllocator Lru2QCache; // LRU2Q cache policy
        typedef Lru2QCache::Config Lru2QCacheConfig;
        typedef Lru2QCache::ReadHandle Lru2QCacheReadHandle;
        typedef Lru2QCache::Item Lru2QCacheItem;

        CachelibLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~CachelibLocalCache();

        // (1) Check is cached and access validity

        virtual bool isLocalCached(const Key& key) const override;

        // (2) Access local edge cache

        virtual bool getLocalCache(const Key& key, Value& value) const override;
        virtual bool updateLocalCache(const Key& key, const Value& value) override;

        // (3) Local edge cache management

        virtual bool needIndependentAdmit(const Key& key) const override;
        virtual void admitLocalCache(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const override;
        virtual bool evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacity() const override;
    private:
        static const std::string kClassName;

        // (4) Other functions

        virtual void checkPointers_() const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Guarantee the atomicity of local Cachelib cache and local statistics
        mutable Rwlock* rwlock_for_cachelib_local_cache_ptr_;

        // Non-const shared variables
        std::unique_ptr<Lru2QCache> cachelib_cache_ptr_; // Data and metadata for local edge cache
        facebook::cachelib::PoolId cachelib_poolid_; // Pool ID for cachelib local edge cache
    };
}

#endif