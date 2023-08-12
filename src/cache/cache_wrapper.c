#include "cache/cache_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string CacheWrapper::kClassName("CacheWrapper");

    CacheWrapper::CacheWrapper(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Allocate local edge cache
        local_cache_ptr_ = LocalCacheBase::getLocalCacheByCacheName(cache_name, edge_idx, capacity_bytes);
        assert(local_cache_ptr_ != NULL);

        // Allocate per-key rwlock for cache wrapper
        cache_wrapper_perkey_rwlock_ptr_ = new PerkeyRwlock(edge_idx, Config::getFineGrainedLockingSize(), local_cache_ptr_->hasFineGrainedManagement());
        assert(cache_wrapper_perkey_rwlock_ptr_ != NULL);

        // Allocate validity map
        validity_map_ptr_ = new ValidityMap(edge_idx, cache_wrapper_perkey_rwlock_ptr_);
        assert(validity_map_ptr_ != NULL);
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

        bool is_exist = false;
        validity_map_ptr_->invalidateFlagForKey(key, is_exist);
        if (!is_exist) // a key locally cached is not found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " does not exist in validity_map_ for invalidateIfCached()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }

    // (2) Access local edge cache

    bool CacheWrapper::get(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::get()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool is_local_cached = local_cache_ptr_->getLocalCache(key, value); // Still need to update local statistics if key is cached yet invalid

        bool is_valid = false;
        if (is_local_cached)
        {
            is_valid = isValidKeyForLocalCachedObject_(key);
        }

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return is_local_cached && is_valid;
    }
    
    bool CacheWrapper::update(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::update()";
        if (value.isDeleted())
        {
            context_name = "CacheWrapper::remove()";
        }
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        bool is_local_cached = local_cache_ptr_->updateLocalCache(key, value);

        if (is_local_cached)
        {
            validateKeyForLocalCachedObject_(key);
        }

        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);
        return is_local_cached;
    }

    bool CacheWrapper::remove(const Key& key)
    {
        // No need to acquire a write lock, which will be done in update()

        Value deleted_value;
        bool is_local_cached = update(key, deleted_value);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool CacheWrapper::needIndependentAdmit(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock
        std::string context_name = "CacheWrapper::needIndependentAdmit()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock_shared(key, context_name);

        bool need_independent_admit = local_cache_ptr_->needIndependentAdmit(key);

        // Release a read lock
        cache_wrapper_perkey_rwlock_ptr_->unlock_shared(key, context_name);

        return need_independent_admit;
    }

    void CacheWrapper::admit(const Key& key, const Value& value, const bool& is_valid)
    {
        checkPointers_();

        // Acquire a write lock
        std::string context_name = "CacheWrapper::admit()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        local_cache_ptr_->admitLocalCache(key, value);

        if (is_valid) // w/o writes
        {
            validateKeyForLocalUncachedObject_(key);
        }
        else // w/ writes
        {
            invalidateKeyForLocalUncachedObject_(key);
        }

        // Release a write lock
        cache_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return;
    }
    
    void CacheWrapper::evict(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value)
    {
        checkPointers_();

        if (local_cache_ptr_->hasFineGrainedManagement()) // Local cache with fine-grained management
        {
            evictForFineGrainedManagement_(keys, values, admit_key, admit_value);
        }
        else // Local cache with coarse-grained management
        {
            evictForCoarseGrainedManagement_(keys, values, admit_key, admit_value);
        }

        return;
    }

    // (4) Other functions

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

    void CacheWrapper::validateKeyForLocalUncachedObject_(const Key& key)
    {
        // No need to acquire a write lock, which has been done in public functions

        bool is_exist = false;
        validity_map_ptr_->validateFlagForKey(key, is_exist);
        if (is_exist) // a key not locally cached is found in validity_map_
        {
            std::ostringstream oss;
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for validateIfUncached_()";
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
            oss << "key " << key.getKeystr() << " already exists in validity_map_ for invalidateIfUncached_()";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        return;
    }

    // (3) Local edge cache management

    void CacheWrapper::evictForFineGrainedManagement_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key = Key(), const Value& admit_value = Value())
    {
        assert(local_cache_ptr_->hasFineGrainedManagement());

        // NOTE: keys and values are uninitialized now!!!
        keys.clear();
        values.clear();

        // NOTE: we should NOT modify local_cache_ptr_ outside the write lock, otherwise we cannot guarantee atomicity/order between local_cache_ptr_ and validity_map_ptr_, which may incur an inconsistent state

        while (true)
        {
            // Get victim_key for key-level fine-grained locking
            Key victim_key;
            bool has_victim_key = local_cache_ptr_->getLocalCacheVictimKey(victim_key, admit_key, admit_value);
            Value victim_value;

            // At least one victim key should exist for eviction
            if (!has_victim_key)
            {
                Util::dumpErrorMsg(instance_name_, "NOT find any victim key for evict()!");
                exit(1);
            }

            // Acquire a write lock (pessimistic locking to avoid atomicity/order issues)
            std::string context_name = "CacheWrapper::evictForFineGrainedManagement_()";
            cache_wrapper_perkey_rwlock_ptr_->acquire_lock(victim_key, context_name);

            // Evict if key matches (similar as version check for optimistic locking to revert effects of atomicity/order issues)
            bool is_evict = local_cache_ptr_->evictLocalCacheIfKeyMatch(victim_key, victim_value, admit_key, admit_value);
            if (is_evict) // If with successful eviction
            {
                bool is_exist = false;
                validity_map_ptr_->eraseFlagForKey(victim_key, is_exist);
                if (!is_exist)
                {
                    std::ostringstream oss;
                    oss << "victim key " << victim_key.getKeystr() << " does not exist in validity_map_ for evict()";
                    Util::dumpWarnMsg(instance_name_, oss.str());
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "victim key " << victim_key.getKeystr() << " is not matched during eviction in local_cache_ptr_ for evict()!";
                Util::dumpWarnMsg(instance_name_, oss.str());
            }

            // Release a write lock
            cache_wrapper_perkey_rwlock_ptr_->unlock(victim_key, context_name);

            if (is_evict) // If with successful eviction
            {
                keys.push_back(victim_key);
                values.push_back(victim_value);
                break;
            }
        }

        return;
    }

    void CacheWrapper::evictForCoarseGrainedManagement_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key = Key(), const Value& admit_value = Value())
    {
        assert(!local_cache_ptr_->hasFineGrainedManagement());

        // NOTE: keys and values are uninitialized now!!!
        keys.clear();
        values.clear();

        // Acquire a write lock (pessimistic locking to avoid atomicity/order issues)
        std::string context_name = "CacheWrapper::evictForCoarseGrainedManagement_()";
        cache_wrapper_perkey_rwlock_ptr_->acquire_lock(Key(), context_name); // NOTE: parameter key will NOT be used due to NOT using fine-grained locking for coarse-grained management

        // Directly evict local cache for coarse-grained management
        local_cache_ptr_->evictLocalCache(keys, values, admit_key, admit_value);
        for (uint32_t i = 0; i < keys.size(); i++)
        {
            bool is_exist = false;
            validity_map_ptr_->eraseFlagForKey(keys[i], is_exist);
            if (!is_exist)
            {
                std::ostringstream oss;
                oss << "victim keys[" << i << "] " << keys[i].getKeystr() << " does not exist in validity_map_ for evict()";
                Util::dumpWarnMsg(instance_name_, oss.str());
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
    }
}