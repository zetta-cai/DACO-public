#include "cache/lru_local_cache.h"

#include <assert.h>
#include <sstream>

namespace covered
{
    const std::string LruLocalCache::kClassName("LruLocalCache");

    LruLocalCache::LruLocalCache(const uint32_t& edge_idx) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_lru_local_cache_ptr_";
        rwlock_for_lru_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_lru_local_cache_ptr_ != NULL);

        lru_cache_ptr_ = new LruCache();
        assert(lru_cache_ptr_ != NULL);
    }
    
    LruLocalCache::~LruLocalCache()
    {
        assert(rwlock_for_lru_local_cache_ptr_ != NULL);
        delete rwlock_for_lru_local_cache_ptr_;
        rwlock_for_lru_local_cache_ptr_ = NULL;

        assert(lru_cache_ptr_ != NULL);
        delete lru_cache_ptr_;
        lru_cache_ptr_ = NULL;
    }

    // (1) Check is cached and access validity

    bool LruLocalCache::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::isLocalCached()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock_shared(context_name);

        bool is_cached = lru_cache_ptr_->exists(key);

        rwlock_for_lru_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache

    bool LruLocalCache::getLocalCache(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::getLocalCache()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lru_cache_ptr_->get(key, value);

        rwlock_for_lru_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    bool LruLocalCache::updateLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::updateLocalCache()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lru_cache_ptr_->update(key, value);

        rwlock_for_lru_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LruLocalCache::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void LruLocalCache::admitLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::admitLocalCache()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock(context_name);

        lru_cache_ptr_->admit(key, value);

        rwlock_for_lru_local_cache_ptr_->unlock(context_name);
        return;
    }

    void LruLocalCache::evictLocalCache(Key& key, Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::evictLocalCache()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock(context_name);

        lru_cache_ptr_->evict(key, value);

        rwlock_for_lru_local_cache_ptr_->unlock(context_name);
        return;
    }

    // (4) Other functions

    uint64_t LruLocalCache::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LruLocalCache::getSizeForCapacity()";
        rwlock_for_lru_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = lru_cache_ptr_->getSizeForCapacity();

        rwlock_for_lru_local_cache_ptr_->unlock_shared(context_name);
        return internal_size;
    }

    void LruLocalCache::checkPointers_() const
    {
        assert(rwlock_for_lru_local_cache_ptr_ != NULL);
        assert(lru_cache_ptr_ != NULL);
    }

}