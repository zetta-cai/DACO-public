/*
 * LocalCacheBase: base class of local edge cache (thread safe).
 * 
 * NOTE: all non-const shared variables in CacheWrapperBase should be thread safe (guaranteed by rwlock_for_local_cache_ptr_).
 * 
 * By Siyuan Sheng (2023.06.30).
 */

#ifndef LOCAL_CACHE_BASE_H
#define LOCAL_CACHE_BASE_H

#include <list>
#include <set>
#include <string>
#include <vector>

#include "common/key.h"
#include "common/value.h"
#include "concurrency/rwlock.h"
#include "core/victim/victim_info.h"

namespace covered
{
    class LocalCacheBase
    {
    public:
        static LocalCacheBase* getLocalCacheByCacheName(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint32_t& peredge_synced_victimcnt);

        LocalCacheBase(const uint32_t& edge_idx);
        virtual ~LocalCacheBase();

        // (1) Check is cached and access validity

        bool isLocalCached(const Key& key) const;

        // (2) Access local edge cache (KV data and local metadata)

        bool getLocalCache(const Key& key, Value& value, bool& affect_victim_tracker) const; // Return whether key is cached
        std::list<VictimInfo> getLocalSyncedVictimInfosFromLocalCache() const; // Return up to peredge_synced_victimcnt local synced victims with the least local rewards

        bool updateLocalCache(const Key& key, const Value& value, bool& affect_victim_tracker); // Return whether key is cached

        void updateLocalUncachedMetadataForRsp(const Key& key, const Value& value, const bool& is_value_related) const; // Triggered by get/put/delrsp for cache miss for admission policy if any

        // (3) Local edge cache management

        // If get() or update() or remove() in CacheWrapper returns false (i.e., key is not cached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/o admission policy (i.e., always admit) will always return true if key is not cached, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        bool needIndependentAdmit(const Key& key) const;

        void admitLocalCache(const Key& key, const Value& value);

        // If local cache supports fine-grained cache management, split evict() into two steps for key-level fine-grained locking in cache wrapper: (i) get victim key; (ii) evict if victim key matches similar as version check
        bool getLocalCacheVictimKeys(std::set<Key>& keys, const uint64_t& required_size) const; // Return false if no victim key (for fine-grained management)
        bool evictLocalCacheWithGivenKey(const Key& key, Value& value); // Return false if key does NOT exist (for fine-grained management)

        // If local cache only supports coarse-grained cache management, evict local cache directly
        void evictLocalCacheNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // For coarse-grained management

        // (4) Other functions
        
        // In units of bytes
        uint64_t getSizeForCapacity() const; // Get size of data and metadata for local edge cache

        virtual const bool hasFineGrainedManagement() const = 0; // Whether the local edge cache supports key-level (i.e., object-level) fine-grained cache management
    protected:
        // (4) Other functions

        void checkPointers_() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const = 0;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const = 0; // Return whether key is cached
        virtual std::list<VictimInfo> getLocalSyncedVictimInfosFromLocalCacheInternal_() const = 0; // Return up to peredge_synced_victimcnt local synced victims with the least local rewards

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker) = 0; // Return whether key is cached

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const = 0; // Triggered by get/put/delrsp for cache miss for admission policy if any

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const = 0;

        virtual void admitLocalCacheInternal_(const Key& key, const Value& value) = 0;
        virtual bool getLocalCacheVictimKeysInternal_(std::set<Key>& keys, const uint64_t& required_size) const = 0;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) = 0;

        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) = 0;

        // (4) Other functions

        virtual uint64_t getSizeForCapacityInternal_() const = 0; // Get size of data and metadata for local edge cache

        virtual void checkPointersInternal_() const = 0;

        // Member variables

        std::string base_instance_name_; // Const shared variable

        // NOTE: we use a single read-write lock for thread safety of local edge cache, including concurrent accesses of cached objects and local metadata with fair comparison
        // NOTE: we do NOT hack each cache for fine-grained locking to avoid extensive complexity
        mutable Rwlock* rwlock_for_local_cache_ptr_; // Guarantee the atomicity of local edge cache and local metadata
    };
}

#endif