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
#include <string>
#include <vector>

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
        CacheWrapper(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt);
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
        bool update(const Key& key, const Value& value, bool& affect_victim_tracker);
        bool remove(const Key& key, bool& affect_victim_tracker);

        // Return whether key is cached yet invalid
        bool updateIfInvalidForGetrsp(const Key& key, const Value& value, bool& affect_victim_tracker); // Update value only if key is locally cached yet invalid
        bool removeIfInvalidForGetrsp(const Key& key, bool& affect_victim_tracker); // Remove value only if it is locally cached yet invalid

        // Return up to peredge_synced_victimcnt local synced victims with the least local rewards
        std::list<VictimCacheinfo> getLocalSyncedVictimCacheinfos() const;

        // Return if exist victims for the required size
        bool fetchVictimCacheinfosForRequiredSize(std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const;

        // Set collected_popularity.is_tracked_ as true if the local uncached key is tracked; set collected_popularity.is_tracked_ as false if key is either local cached or local uncached yet untracked by local uncached metadata
        // NOTE: for directory lookup req, directory eviction req, acquire writelock req, and release writelock req, is_key_tracked flag could still be false for returned collected popularity -> reason: under local uncached metadata capacity limitation, newly-tracked or preserved-after-eviciton local uncached popularity could be immediately detracked from local uncached metadata and hence NO need for popularity collection/aggregation
        void getCollectedPopularity(const Key& key, CollectedPopularity& collected_popularity) const;

        // (3) Local edge cache management

        bool needIndependentAdmit(const Key& key, const Value& value) const;
        void admit(const Key& key, const Value& value, const bool& is_valid, bool& affect_victim_tracker);
        void evict(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // NOTE: single-thread function; eviction MUST affect victim tracker due to evicting objects with least local rewards (i.e., local synced victims)

        // (4) Other functions
        
        // In units of bytes
        uint64_t getSizeForCapacity() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)
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

        std::string instance_name_; // Const shared variable
        const std::string cache_name_; // Come from CLI

        // Non-const shared variable
        LocalCacheBase* local_cache_ptr_; // Maintain key-value objects for local edge cache (thread safe)
        // Fine-graind locking for local edge cache with fine-grained management, or single read-write lock for that with coarse-grained cache management
        mutable PerkeyRwlock* cache_wrapper_perkey_rwlock_ptr_;
        // NOTE: Due to the write-through policy, we only need to maintain an invalidity flag for MSI protocol (i.e., both M and S refers to validity)
        ValidityMap* validity_map_ptr_; // Maintain per-key validity flag for local edge cache (thread safe)

        #ifdef DEBUG_CACHE_WRAPPER
        // For debugging
        mutable Rwlock* cache_wrapper_rwlock_for_debug_ptr_;
        std::unordered_set<Key, KeyHasher> cached_keys_for_debug_;
        #endif
    };
}

#endif