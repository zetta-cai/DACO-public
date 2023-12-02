#include "cache/cache_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string CacheWrapper::kClassName("CacheWrapper");

    CacheWrapper::CacheWrapper(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt) : cache_name_(cache_name)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Allocate local edge cache
        local_cache_ptr_ = LocalCacheBase::getLocalCacheByCacheName(cache_name, edge_idx, capacity_bytes, local_uncached_capacity_bytes, peredge_synced_victimcnt);
        assert(local_cache_ptr_ != NULL);

        // Allocate per-key rwlock for cache wrapper
        cache_wrapper_perkey_rwlock_ptr_ = new PerkeyRwlock(edge_idx, Config::getFineGrainedLockingSize(), local_cache_ptr_->hasFineGrainedManagement());
        assert(cache_wrapper_perkey_rwlock_ptr_ != NULL);

        // Allocate validity map
        validity_map_ptr_ = new ValidityMap(edge_idx, cache_wrapper_perkey_rwlock_ptr_);
        assert(validity_map_ptr_ != NULL);

        #ifdef DEBUG_CACHE_WRAPPER
        cache_wrapper_rwlock_for_debug_ptr_ = new Rwlock("cache_wrapper_rwlock_for_debug_ptr_");
        cached_keys_for_debug_.clear();
        #endif
    }
    
    CacheWrapper::~CacheWrapper()
    {
        // Release local edge cache
        assert(local_cache_ptr_ != NULL);
        delete local_cache_ptr_;
        local_cache_ptr_ = NULL;

        // Release per-key rwlock for cache wrapper
        assert(cache_wrapper_perkey_rwlock_ptr_ != NULL);
        delete cache_wrapper_perkey_rwlock_ptr_;
        cache_wrapper_perkey_rwlock_ptr_ = NULL;

        // Release validity map
        assert(validity_map_ptr_ != NULL);
        delete validity_map_ptr_;
        validity_map_ptr_ = NULL;

        #ifdef DEBUG_CACHE_WRAPPER
        assert(cache_wrapper_rwlock_for_debug_ptr_ != NULL);
        delete cache_wrapper_rwlock_for_debug_ptr_;
        cache_wrapper_rwlock_for_debug_ptr_ = NULL;
        #endif
    }

    // (1) Check is cached and access validity

    bool CacheWrapper::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::isLocalCached()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool is_local_cached = local_cache_ptr_->isLocalCached(key);

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return is_local_cached;
    }

    bool CacheWrapper::isValidKeyForLocalCachedObject(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::isValidKeyForLocalCachedObject()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool is_valid = isValidKeyForLocalCachedObject_(key);

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return is_valid;
    }

    void CacheWrapper::invalidateKeyForLocalCachedObject(const Key& key)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::invalidateKeyForLocalCachedObject()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        invalidateKeyForLocalCachedObject_(key);

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool CacheWrapper::get(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::get()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool is_local_cached = local_cache_ptr_->getLocalCache(key, is_redirected, value, affect_victim_tracker); // Still need to update local metadata if key is cached yet invalid

        bool is_valid = false;
        if (is_local_cached)
        {
            is_valid = isValidKeyForLocalCachedObject_(key);
        }

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return is_local_cached && is_valid;
    }
    
    bool CacheWrapper::update(const Key& key, const Value& value, const bool& is_global_cached, bool& affect_victim_tracker)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::update()";
        if (value.isDeleted())
        {
            context_name = "CacheWrapper::remove()";
        }
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        // Try to update local edge cache for put/delreq
        const bool is_getrsp = false;
        bool is_successful = false;
        bool is_local_cached = local_cache_ptr_->updateLocalCache(key, value, is_getrsp, is_global_cached, affect_victim_tracker, is_successful);

        if (is_local_cached)
        {
            if (is_successful) // Validate key ONLY if update successfully
            {
                validateKeyForLocalCachedObject_(key);
            }
            else // Invalidate otherwise
            {
                // NOTE: ONLY Segcache will fail to update due to object size > segment size
                assert(cache_name_ == Util::SEGCACHE_CACHE_NAME);

                invalidateKeyForLocalCachedObject_(key);
            }
        }
        else
        {
            assert(!is_successful);
        }

        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);
        return is_local_cached;
    }

    bool CacheWrapper::remove(const Key& key, const bool& is_global_cached, bool& affect_victim_tracker)
    {
        // No need to acquire a write lock, which will be done in update()

        Value deleted_value;
        bool is_local_cached = update(key, deleted_value, is_global_cached, affect_victim_tracker);
        return is_local_cached;
    }

    bool CacheWrapper::updateIfInvalidForGetrsp(const Key& key, const Value& value, const bool& is_global_cached, bool& affect_victim_tracker)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::updateIfInvalidForGetrsp()";
        if (value.isDeleted())
        {
            context_name = "CacheWrapper::remove()";
        }
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        bool is_local_cached_and_invalid = false;

        bool is_local_cached = local_cache_ptr_->isLocalCached(key);
        const bool is_getrsp = true;
        if (is_local_cached)
        {
            bool is_valid = isValidKeyForLocalCachedObject_(key);
            if (!is_valid) // If key is locally cached yet invalid
            {
                // Update local edge cache for getrsp with invalid hit
                bool is_successful = false;
                const bool tmp_is_global_cached = true; // Local cached objects MUST be global cached
                bool tmp_is_local_cached = local_cache_ptr_->updateLocalCache(key, value, is_getrsp, tmp_is_global_cached, affect_victim_tracker, is_successful);
                assert(tmp_is_local_cached);

                if (is_successful) // Validate key ONLY if update successfully
                {
                    validateKeyForLocalCachedObject_(key);
                }
                else // Invalidate otherwise
                {
                    // NOTE: ONLY Segcache will fail to update due to object size > segment size
                    assert(cache_name_ == Util::SEGCACHE_CACHE_NAME);

                    invalidateKeyForLocalCachedObject_(key);
                }

                is_local_cached_and_invalid = true;
            }
        }
        else // If key is NOT local cached
        {
            // Update local edge cache for getrsp with cache miss
            bool is_successful = false;
            bool tmp_is_local_cached = local_cache_ptr_->updateLocalCache(key, value, is_getrsp, is_global_cached, affect_victim_tracker, is_successful);
            assert(!tmp_is_local_cached);
            assert(!is_successful);
        }

        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);
        return is_local_cached_and_invalid;
    }

    bool CacheWrapper::removeIfInvalidForGetrsp(const Key& key, const bool& is_global_cached, bool& affect_victim_tracker)
    {
        // No need to acquire a write lock, which will be done in updateIfInvalidForGetrsp()

        Value deleted_value;
        bool is_local_cached_and_invalid = updateIfInvalidForGetrsp(key, deleted_value, is_global_cached, affect_victim_tracker);
        return is_local_cached_and_invalid;
    }

    std::list<VictimCacheinfo> CacheWrapper::getLocalSyncedVictimCacheinfos() const
    {
        checkPointers_();

        // NOTE: as we only access local edge cache (thread safe w/o per-key rwlock) instead of validity map (thread safe w/ per-key rwlock), we do NOT need to acquire a fine-grained read lock here

        // Acquire a read lock
        //std::string context_name = "CacheWrapper::getLocalSyncedVictimCacheinfos()";
        //cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        std::list<VictimCacheinfo> local_synced_victim_cacheinfos = local_cache_ptr_->getLocalSyncedVictimCacheinfosFromLocalCache(); // NOT update local metadata

        // Release a read lock
        //cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return local_synced_victim_cacheinfos;
    }

    bool CacheWrapper::fetchVictimCacheinfosForRequiredSize(std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        checkPointers_();

        // NOTE: as we only access local edge cache (thread safe w/o per-key rwlock) instead of validity map (thread safe w/ per-key rwlock), we do NOT need to acquire a fine-grained read lock here

        // Acquire a read lock
        //std::string context_name = "CacheWrapper::fetchVictimCacheinfosForRequiredSize()";
        //cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        std::unordered_set<Key, KeyHasher> tmp_victim_keys;
        bool has_victim_key = local_cache_ptr_->getLocalCacheVictimKeys(tmp_victim_keys, victim_cacheinfos, required_size);
        UNUSED(tmp_victim_keys);

        // Release a read lock
        //cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return has_victim_key;
    }

    void CacheWrapper::getCollectedPopularity(const Key& key, CollectedPopularity& collected_popularity) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::getCollectedPopularity()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        local_cache_ptr_->getCollectedPopularityFromLocalCache(key, collected_popularity);

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return;
    }

    // (3) Local edge cache management

    bool CacheWrapper::needIndependentAdmit(const Key& key, const Value& value) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::needIndependentAdmit()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool need_independent_admit = local_cache_ptr_->needIndependentAdmit(key, value);

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return need_independent_admit;
    }

    void CacheWrapper::admit(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid, bool& affect_victim_tracker)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::admit()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        bool is_successful = false;
        local_cache_ptr_->admitLocalCache(key, value, is_neighbor_cached, affect_victim_tracker, is_successful);

        if (is_successful) // If key is admited successfully
        {
            if (is_valid) // w/o writes
            {
                validateKeyForLocalUncachedObject_(key);
            }
            else // w/ writes
            {
                invalidateKeyForLocalUncachedObject_(key);
            }
        }
        // NOTE: NO need to add validity flag if admission fails

        #ifdef DEBUG_CACHE_WRAPPER
        cache_wrapper_rwlock_for_debug_ptr_->acquire_lock(context_name);

        if (cached_keys_for_debug_.find(key) == cached_keys_for_debug_.end())
        {
            cached_keys_for_debug_.insert(key);
        }

        // Used for debugging
        std::ostringstream oss;
        oss << "After admit, " << cached_keys_for_debug_.size() << " cached keys:";
        for (std::set<Key>::const_iterator tmp_iter = cached_keys_for_debug_.begin(); tmp_iter != cached_keys_for_debug_.end(); tmp_iter++)
        {
            oss << " " << tmp_iter->getKeystr() << ";";
        }
        Util::dumpDebugMsg(instance_name_, oss.str());

        cache_wrapper_rwlock_for_debug_ptr_->unlock(context_name);
        #endif

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }
    
    // NOTE: single-thread function
    void CacheWrapper::evict(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        checkPointers_();

        if (local_cache_ptr_->hasFineGrainedManagement()) // Local cache with fine-grained management
        {
            evictForFineGrainedManagement_(victims, required_size);
        }
        else // Local cache with coarse-grained management
        {
            evictForCoarseGrainedManagement_(victims, required_size);
        }

        #ifdef DEBUG_CACHE_WRAPPER
        std::string context_name = "CacheWrapper::evict()";
        cache_wrapper_rwlock_for_debug_ptr_->acquire_lock(context_name);

        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator victims_const_iter = victims.begin(); victims_const_iter != victims.end(); victims_const_iter++)
        {
            if (cached_keys_for_debug_.find(victims_const_iter->first) != cached_keys_for_debug_.end())
            {
                cached_keys_for_debug_.erase(victims_const_iter->first);
            }
        }

        // Used for debugging
        std::ostringstream oss;
        oss << "After evict, " << cached_keys_for_debug_.size() << " cached keys:";
        for (std::set<Key>::const_iterator tmp_iter = cached_keys_for_debug_.begin(); tmp_iter != cached_keys_for_debug_.end(); tmp_iter++)
        {
            oss << " " << tmp_iter->getKeystr() << ";";
        }
        Util::dumpDebugMsg(instance_name_, oss.str());

        cache_wrapper_rwlock_for_debug_ptr_->unlock(context_name);
        #endif

        return;
    }

    // (4) Other functions

    void CacheWrapper::metadataUpdate(const Key& key, const std::string& func_name, const void* func_param_ptr)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::metadataUpdate()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        local_cache_ptr_->updateLocalCacheMetadata(key, func_name, func_param_ptr);

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);
        return;
    }

    uint64_t CacheWrapper::getSizeForCapacity() const
    {
        checkPointers_();

        // No need to acquire a read lock due to no key provided and not critical for size
        
        uint64_t local_edge_cache_size = local_cache_ptr_->getSizeForCapacity();
        uint64_t validity_map_size = validity_map_ptr_->getSizeForCapacity();
        uint64_t total_size = Util::uint64Add(local_edge_cache_size, validity_map_size);

        return total_size;
    }

    // (1) Check is cached and access validity

    bool CacheWrapper::isValidKeyForLocalCachedObject_(const Key& key) const
    {
        // No need to acquire a read lock, which has been done in public functions

        bool is_exist = false;
        bool is_valid = validity_map_ptr_->isValidFlagForKey(key, is_exist);
        if (!is_exist) // key is locally cached yet not found in validity_map_, which may due to processing order issue
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " is locally cached yet not found in validity_map_!";
            Util::dumpWarnMsg(instance_name_, oss.str());
            assert(is_valid == false);
        }

        return is_valid;
    }

    void CacheWrapper::validateKeyForLocalCachedObject_(const Key& key)
    {
        // No need to acquire a write lock, which has been done in public functions

        bool is_exist = false;
        validity_map_ptr_->validateFlagForKey(key, is_exist);
        if (!is_exist) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for validateIfCached()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapper::invalidateKeyForLocalCachedObject_(const Key& key)
    {
        // No need to acquire a write lock, which has been done in public functions

        bool is_exist = false;
        validity_map_ptr_->invalidateFlagForKey(key, is_exist);
        if (!is_exist) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for invalidateKeyForLocalCachedObject_()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapper::validateKeyForLocalUncachedObject_(const Key& key)
    {
        // No need to acquire a write lock, which has been done in public functions

        bool is_exist = false;
        validity_map_ptr_->validateFlagForKey(key, is_exist);
        if (is_exist) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for validateKeyForLocalUncachedObject_()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        return;
    }

    void CacheWrapper::invalidateKeyForLocalUncachedObject_(const Key& key)
    {
        // No need to acquire a write lock, which has been done in public functions

        bool is_exist = false;
        validity_map_ptr_->invalidateFlagForKey(key, is_exist);
        if (is_exist) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for invalidateKeyForLocalUncachedObject_()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        return;
    }

    // (3) Local edge cache management

    // NOTE: single-thread function
    void CacheWrapper::evictForFineGrainedManagement_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(local_cache_ptr_->hasFineGrainedManagement());

        // NOTE: victims is uninitialized now!!!
        victims.clear();

        // NOTE: we should NOT modify local_cache_ptr_ outside the write lock, otherwise we cannot guarantee atomicity/order between local_cache_ptr_ and validity_map_ptr_, which may incur an inconsistent state

        // NOTE: we do NOT retry here (instead we do it in cache server worker), as we need to update required_size based on cache size usage (including local edge cache and cooperation info) and cache capacity

        // Get victim keys for key-level fine-grained locking
        std::unordered_set<Key, KeyHasher> tmp_victim_keys;
        std::list<VictimCacheinfo> tmp_victim_cacheinfos;
        bool has_victim_key = local_cache_ptr_->getLocalCacheVictimKeys(tmp_victim_keys, tmp_victim_cacheinfos, required_size);
        UNUSED(tmp_victim_cacheinfos); // victim_cacheinfos is ONLY used for COVERED's lazy victim fetching, yet NOT used for local edge cache eviction

        // At least one victim key should exist for eviction
        if (!has_victim_key)
        {
            Util::dumpErrorMsg(instance_name_, "NOT find any victim key for evict()!");
            exit(1);
        }
        assert(tmp_victim_keys.size() > 0);

        std::vector<Value> tmp_victim_values;
        for (std::unordered_set<Key, KeyHasher>::const_iterator tmp_victim_key_iter = tmp_victim_keys.begin(); tmp_victim_key_iter != tmp_victim_keys.end(); tmp_victim_key_iter++)
        {
            const Key& tmp_victim_key = *tmp_victim_key_iter;
            Value tmp_victim_value;

            // Acquire a write lock (pessimistic locking to avoid atomicity/order issues)
            // NOTE: we still need to acquire fine-grained locking for tmp_victim_key even if we have acquired cache eviction mutex in cache server worker, otherwise tmp_victim_key may be accessed/motified/invalidated during eviction
            std::string context_name = "CacheWrapper::evictForFineGrainedManagement_()";
            cache_wrapper_perkey_rwlock_ptr_->acquire_lock(tmp_victim_key, context_name);

            // Evict if key matches (similar as version check for optimistic locking to revert effects of atomicity/order issues)
            bool is_evict = local_cache_ptr_->evictLocalCacheWithGivenKey(tmp_victim_key, tmp_victim_value);
            if (is_evict) // If with successful eviction
            {
                bool is_exist = false;
                validity_map_ptr_->eraseFlagForKey(tmp_victim_key, is_exist);
                if (!is_exist)
                {
                    // NOTE: each victim key is cached before eviction, so it MUST exist in validity_map_
                    std::ostringstream oss;
                    oss << "victim key " << tmp_victim_key.getKeystr() << " does not exist in validity_map_ for evictForFineGrainedManagement_()";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                victims.insert(std::pair<Key, Value>(tmp_victim_key, tmp_victim_value));
            }
            else
            {
                // NOTE: as there exists up to one thread to perform eviction each time (single-thread function), each victim key MUST exist in local edge cache
                std::ostringstream oss;
                oss << "victim key " << tmp_victim_key.getKeystr() << " does not exist in local_cache_ptr_ for evictForFineGrainedManagement_()!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Release a write lock
            cache_wrapper_perkey_rwlock_ptr_->unlock(tmp_victim_key, context_name);
        }

        return;
    }

    // NOTE: single-thread function
    void CacheWrapper::evictForCoarseGrainedManagement_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!local_cache_ptr_->hasFineGrainedManagement());

        // NOTE: victims is uninitialized now!!!
        victims.clear();

        // Acquire a write lock (pessimistic locking to avoid atomicity/order issues)
        std::string context_name = "CacheWrapper::evictForCoarseGrainedManagement_()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(Key(), context_name); // NOTE: parameter key will NOT be used due to NOT using fine-grained locking for coarse-grained management

        // Directly evict local cache for coarse-grained management
        local_cache_ptr_->evictLocalCacheNoGivenKey(victims, required_size);
        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator victim_iter = victims.begin(); victim_iter != victims.end(); victim_iter++)
        {
            const Key& tmp_victim_key = victim_iter->first;

            bool is_exist = false;
            validity_map_ptr_->eraseFlagForKey(tmp_victim_key, is_exist);
            if (!is_exist)
            {
                // NOTE: each victim key is cached before eviction, so it MUST exist in validity_map_
                std::ostringstream oss;
                oss << "victim key " << tmp_victim_key.getKeystr() << " does not exist in validity_map_ for evictForCoarseGrainedManagement_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(Key(), context_name);

        return;
    }

    // (4) Other functions

    void CacheWrapper::checkPointers_() const
    {
        assert(cache_wrapper_perkey_rwlock_ptr_ != NULL);
        assert(validity_map_ptr_ != NULL);
        assert(local_cache_ptr_ != NULL);

        #ifdef DEBUG_CACHE_WRAPPER
        assert(cache_wrapper_rwlock_for_debug_ptr_ != NULL);
        #endif
    }
}