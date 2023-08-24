/*
 * CachelibLocalCache: local edge cache with LRU2Q policy based on Cachelib (https://github.com/facebook/CacheLib.git).
 * 
 * NOTE: all configuration and function calls refer to Cachelib files, including lib/CacheLib/examples/simple_cache/main.cpp and lib/cachelib/cachebench/runner/CacheStressor.h.
 * 
 * NOTEs for source code of CacheLib
 * (1) For each insert/update, CacheAllocator uses MemoryAllocator to allocate memory for new item (automatic management based on refcnt; only manage memory for key, value, LRU/FIFO and hashtable-lookup hook/pointer, and flags, while metadata such as CMS-based access frequencies are maintained by MMContainer such as MMTinyLFU), uses MM2Q as MMContainer to admit new metadata or evict victim metadata, and uses ChainedHashTable as AccessContainer for add new item into hash table.
 * (2) A handle's it_ points to CacheItem, whose kAllocation alloc_ stores key and value (it encodes key size and value size into one uint32_t variable, and concatenates key bytes and value bytes into one unsigned char array) -> getMemory() and getSize() return value bytes and size, while getKey() return Key (inheriting from folly::StringPiece) with key bytes and size -> deconstructor of handle will release the memory of pointed item if the refcnt is decreased to 0.
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
        typedef Lru2QAllocator Lru2QCache; // LRU2Q cache policy
        typedef Lru2QCache::Config Lru2QCacheConfig;
        typedef Lru2QCache::ReadHandle Lru2QCacheReadHandle;
        typedef Lru2QCache::Item Lru2QCacheItem;

        // NOTE: too small cache capacity cannot support slab-based memory allocation in cachelib (see lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h and lib/CacheLib/cachelib/allocator/memory/SlabAllocator.cpp)
        static const uint64_t CACHELIB_MIN_CAPACITY_BYTES; // NOTE: NOT affect capacity constraint!

        CachelibLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~CachelibLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const override; // Triggered by get/put/delrsp for cache miss for admission policy if any

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::set<Key, KeyHasher>& keys, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        std::unique_ptr<Lru2QCache> cachelib_cache_ptr_; // Data and metadata for local edge cache
        facebook::cachelib::PoolId cachelib_poolid_; // Pool ID for cachelib local edge cache
    };
}

#endif