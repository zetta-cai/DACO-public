#include "cache/segcache_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string SegcacheLocalCache::kClassName("SegcacheLocalCache");

    SegcacheLocalCache::SegcacheLocalCache(const uint32_t& edge_idx) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_segcache_local_cache_ptr_";
        rwlock_for_segcache_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);

        // TODO: END HERE

        lru_cache_ptr_ = new LruCache();
        assert(lru_cache_ptr_ != NULL);
    }
    
    SegcacheLocalCache::~SegcacheLocalCache()
    {
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);
        delete rwlock_for_segcache_local_cache_ptr_;
        rwlock_for_segcache_local_cache_ptr_ = NULL;

        assert(lru_cache_ptr_ != NULL);
        delete lru_cache_ptr_;
        lru_cache_ptr_ = NULL;
    }

    // (1) Check is cached and access validity

    bool SegcacheLocalCache::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::isLocalCached()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        bool is_cached = lru_cache_ptr_->exists(key);

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache

    bool SegcacheLocalCache::getLocalCache(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lru_cache_ptr_->get(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    bool SegcacheLocalCache::updateLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::updateLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lru_cache_ptr_->update(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool SegcacheLocalCache::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void SegcacheLocalCache::admitLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::admitLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        lru_cache_ptr_->admit(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return;
    }

    bool SegcacheLocalCache::getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getLocalCacheVictimKey()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        bool has_victim_key = lru_cache_ptr_->getVictimKey(key);

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return has_victim_key;
    }

    bool SegcacheLocalCache::evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::evictLocalCacheIfKeyMatch()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        bool is_evict = lru_cache_ptr_->evictIfKeyMatch(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_evict;
    }

    // (4) Other functions

    uint64_t SegcacheLocalCache::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getSizeForCapacity()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = lru_cache_ptr_->getSizeForCapacity();

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return internal_size;
    }

    void SegcacheLocalCache::checkPointers_() const
    {
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);
        assert(lru_cache_ptr_ != NULL);
    }

}