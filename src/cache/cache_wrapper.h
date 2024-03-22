/*
 * CacheWrapper: provide general local edge cache interfaces with validity flags (thread safe).
 * 
 * NOTE: all non-const shared variables in CacheWrapperBase should be thread safe.
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#ifndef CACHE_WRAPPER_H
#define CACHE_WRAPPER_H

//#define DEBUG_CACHE_WRAPPER

#include <list>
#include <set>
#include <string>
#include <vector>

#include "cache/cache_custom_func_param_base.h"
#include "concurrency/concurrent_hashtable_impl.h"

namespace covered
{
    // Forward declaration
    class CacheWrapper;
}

#include "cache/local_cache_base.h"
#include "cache/validity_map.h"
#include "common/covered_common_header.h"
#include "common/key.h"
#include "common/value.h"
#include "concurrency/perkey_rwlock.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class CacheWrapper
    {
    public:
        CacheWrapper(const EdgeWrapperBase* edge_wrapper_ptr, const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint32_t& dataset_keycnt, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt);
        virtual ~CacheWrapper();

        // (1) Check is cached and access validity

        bool isLocalCached(const Key& key) const;
        bool isValidKeyForLocalCachedObject(const Key& key) const;
        // For invalidation control requests
        void invalidateKeyForLocalCachedObject(const Key& key); // Add an invalid flag if key NOT exist

        // (2) Access local edge cache (KV data and local metadata)

        // Return whether key is cached and valid (i.e., local cache hit) after get/update/remove
        bool get(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const;

        // Return whether key is cached, while both update() and remove() will set validity as true
        // NOTE: update() only updates the object if cached, yet not admit a new one
        // NOTE: remove() only marks the object as deleted if cached, yet not evict it
        bool update(const Key& key, const Value& value, const bool& is_global_cached, bool& affect_victim_tracker);
        bool remove(const Key& key, const bool& is_global_cached, bool& affect_victim_tracker);

        // Return whether key is cached yet invalid
        bool updateIfInvalidForGetrsp(const Key& key, const Value& value, const bool& is_global_cached, bool& affect_victim_tracker); // Update value only if key is locally cached yet invalid
        bool removeIfInvalidForGetrsp(const Key& key, const bool& is_global_cached, bool& affect_victim_tracker); // Remove value only if it is locally cached yet invalid

        // Return if exist victims for the required size
        bool fetchVictimCacheinfosForRequiredSize(std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const;

        // (3) Local edge cache management

        bool needIndependentAdmit(const Key& key, const Value& value) const;
        void admit(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid, const uint32_t& beacon_edgeidx, bool& affect_victim_tracker);
        void evict(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // NOTE: single-thread function; eviction MUST affect victim tracker due to evicting objects with least local rewards (i.e., local synced victims)

        // (4) Other functions

        void customFunc(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr); // Invoke method-specific functions for local edge cache
        void constCustomFunc(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const;
        void preCustomFunc_(const std::string context_name, CacheCustomFuncParamBase* func_param_ptr) const;
        void postCustomFunc_(const std::string& func_name, const std::string context_name, CacheCustomFuncParamBase* func_param_ptr) const;
        
        // In units of bytes
        uint64_t getSizeForCapacity() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)

        // (5) Dump/load cache snapshot
        virtual void dumpCacheSnapshot(std::fstream* fs_ptr) const;
        virtual void loadCacheSnapshot(std::fstream* fs_ptr);
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        bool isValidKeyForLocalCachedObject_(const Key& key) const;
        // For local put/del requests invoked by update() and remove()
        void validateKeyForLocalCachedObject_(const Key& key); // Add a valid flag if key NOT exist
        // For invalidation control requests and unsuccessful local put/del requests invoked by update() and remove()
        void invalidateKeyForLocalCachedObject_(const Key& key); // Add an invalid flag if key NOT exist
        // For local get/put/del requests invoked by admit() w/o writes
        void validateKeyForLocalUncachedObject_(const Key& key); // Add an invalid flag if key NOT exist
        // For local get/put/del requests invoked by admit() w/ writes
        void invalidateKeyForLocalUncachedObject_(const Key& key); // Add an invalid flag if key NOT exist

        // (3) Local edge cache management

        void evictForFineGrainedManagement_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // NOTE: single-thread function
        void evictForCoarseGrainedManagement_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // NOTE: single-thread function

        // (4) Other functions

        void checkPointers_() const;

        // Member variables

        // Const shared variables
        std::string instance_name_; // Const shared variable
        const std::string cache_name_; // Come from CLI

        // Non-const shared variable
        LocalCacheBase* local_cache_ptr_; // Maintain key-value objects for local edge cache (thread safe)
        // Fine-graind locking for local edge cache with fine-grained management, or single read-write lock for that with coarse-grained cache management
        mutable PerkeyRwlock* cache_wrapper_perkey_rwlock_ptr_;
        // NOTE: Due to the write-through policy, we only need to maintain an invalidity flag for MSI protocol (i.e., both M and S refers to validity)
        ValidityMap* validity_map_ptr_; // Maintain per-key validity flag for local edge cache (thread safe)

        // Impl trick to avoid duplicate consistent hashing
        mutable Rwlock* cache_wrapper_rwlock_for_beacon_edgeidx_ptr_;
        std::unordered_map<Key, uint32_t, KeyHasher> perkey_beacon_edgeidx_;

        #ifdef DEBUG_CACHE_WRAPPER
        // For debugging
        mutable Rwlock* cache_wrapper_rwlock_for_debug_ptr_;
        std::set<Key> cached_keys_for_debug_;
        #endif
    };
}

#endif