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
#include <string>
#include <unordered_set>
#include <vector>

#include "cache/cache_custom_func_param_base.h"

namespace covered
{
    // Forward declaration
    class LocalCacheBase;
}

#include "common/key.h"
#include "common/value.h"
#include "concurrency/rwlock.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_cacheinfo.h"
#include "edge/edge_wrapper_base.h"

namespace covered
{
    class LocalCacheBase
    {
    public:
        static LocalCacheBase* getLocalCacheByCacheName(const EdgeWrapperBase* edge_wrapper_ptr, const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt);

        LocalCacheBase(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~LocalCacheBase();

        // (1) Check is cached and access validity

        bool isLocalCached(const Key& key) const;

        // (2) Access local edge cache (KV data and local metadata)

        bool getLocalCache(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const; // Return whether key is cached

        bool updateLocalCache(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful); // Return whether key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        // If get() or update() or remove() in CacheWrapper returns false (i.e., key is not cached), EdgeWrapper will invoke needIndependentAdmit() for admission policy
        // NOTE: cache methods w/o admission policy (i.e., always admit) will always return true if key is not cached, while others will return true/false based on other independent admission policy
        // NOTE: only COVERED never needs independent admission (i.e., always returns false)
        bool needIndependentAdmit(const Key& key, const Value& value) const;

        void admitLocalCache(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful); // is_successful indicates whether object is admited successfully

        // If local cache supports fine-grained cache management, split evict() into two steps for key-level fine-grained locking in cache wrapper: (i) get victim key; (ii) evict if victim key matches similar as version check
        // NOTE: keys is used for local edge cache eviction, while victim_cacheinfos is used for lazy victim fetching (for COVERED)
        bool getLocalCacheVictimKeys(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const; // Return false if no victim key (for fine-grained management)
        bool evictLocalCacheWithGivenKey(const Key& key, Value& value); // Return false if key does NOT exist (for fine-grained management)

        // If local cache only supports coarse-grained cache management, evict local cache directly
        void evictLocalCacheNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // For coarse-grained management

        // (4) Other functions

        void invokeCustomFunction(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr); // Invoke some method-specific function for local edge cache
        void invokeConstCustomFunction(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const; // Invoke some method-specific function for local edge cache
        void preInvokeCustomFunction_(const std::string& context_name, CacheCustomFuncParamBase* func_param_ptr) const;
        void postInvokeCustomFunction_(const std::string& context_name, CacheCustomFuncParamBase* func_param_ptr) const;
        
        // In units of bytes
        uint64_t getSizeForCapacity() const; // Get size of data and metadata for local edge cache

        virtual const bool hasFineGrainedManagement() const = 0; // Whether the local edge cache supports key-level (i.e., object-level) fine-grained cache management
    protected:
        const uint64_t capacity_bytes_; // Const variables
        const EdgeWrapperBase* edge_wrapper_ptr_; // ONLY use weight info for local reward calculation

        // (4) Other functions

        void checkPointers_() const;

        // Object size checking
        bool isValidObjsize_(const ObjectSize& objsize) const;
        bool isValidObjsize_(const Key& key, const Value& value) const;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const = 0;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const = 0;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const = 0; // Return whether key is cached

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) = 0; // Return whether key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const = 0;

        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful) = 0; // is_successful indicates whether object is admited successfully
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const = 0;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) = 0;

        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) = 0;

        // (4) Other functions

        virtual void invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) = 0; // Invoke some method-specific function for local edge cache
        virtual void invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const = 0; // Invoke some method-specific function for local edge cache

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