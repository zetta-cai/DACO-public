/*
 * CacheWrapper: provide general local edge cache interfaces with validity flags (thread safe).
 * 
 * NOTE: all non-const shared variables in CacheWrapperBase should be thread safe.
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#ifndef CACHE_WRAPPER_H
#define CACHE_WRAPPER_H

#include <list>
#include <string>
#include <vector>

#include "cache/local_cache_base.h"
#include "cache/validity_map.h"
#include "common/key.h"
#include "common/value.h"
#include "concurrency/perkey_rwlock.h"
#include "core/victim/victim_info.h"

namespace covered
{
    class CacheWrapper
    {
    public:
        CacheWrapper(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint32_t& peredge_synced_victimcnt);
        virtual ~CacheWrapper();

        // (1) Check is cached and access validity

        bool isLocalCached(const Key& key) const;
        bool isValidKeyForLocalCachedObject(const Key& key) const;
        // For invalidation control requests
        void invalidateKeyForLocalCachedObject(const Key& key); // Add an invalid flag if key NOT exist

        // (2) Access local edge cache (KV data and local metadata)

        // Return whether key is cached and valid (i.e., local cache hit) after get/update/remove
        bool get(const Key& key, Value& value, bool& affect_victim_tracker) const;

        // Return whether key is cached, while both update() and remove() will set validity as true
        // NOTE: update() only updates the object if cached, yet not admit a new one
        // NOTE: remove() only marks the object as deleted if cached, yet not evict it
        bool update(const Key& key, const Value& value);
        bool remove(const Key& key);

        // Return whether key is cached yet invalid
        bool updateIfInvalidForGetrsp(const Key& key, const Value& value); // Update value only if key is locally cached yet invalid
        bool removeIfInvalidForGetrsp(const Key& key); // Remove value only if it is locally cached yet invalid

        // Return up to peredge_synced_victimcnt local synced victims with the least local rewards
        std::list<VictimInfo> getLocalSyncedVictimInfos() const;

        // (3) Local edge cache management

        bool needIndependentAdmit(const Key& key) const;
        void admit(const Key& key, const Value& value, const bool& is_valid);
        void evict(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size); // NOTE: single-thread function

        // (4) Other functions
        
        // In units of bytes
        uint64_t getSizeForCapacity() const; // sum of internal size (each individual local cache) and external size (metadata for edge caching)
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        bool isValidKeyForLocalCachedObject_(const Key& key) const;
        // For local put/del requests invoked by update() and remove()
        void validateKeyForLocalCachedObject_(const Key& key); // Add a valid flag if key NOT exist
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

        // Non-const shared variable
        LocalCacheBase* local_cache_ptr_; // Maintain key-value objects for local edge cache (thread safe)
        // Fine-graind locking for local edge cache with fine-grained management, or single read-write lock for that with coarse-grained cache management
        mutable PerkeyRwlock* cache_wrapper_perkey_rwlock_ptr_;
        // NOTE: Due to the write-through policy, we only need to maintain an invalidity flag for MSI protocol (i.e., both M and S refers to validity)
        ValidityMap* validity_map_ptr_; // Maintain per-key validity flag for local edge cache (thread safe)
    };
}

#endif