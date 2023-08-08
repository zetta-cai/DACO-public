#include "cache/lfu_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string LfuLocalCache::kClassName("LfuLocalCache");

    LfuLocalCache::LfuLocalCache(const uint32_t& edge_idx) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_lfu_local_cache_ptr_";
        rwlock_for_lfu_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_lfu_local_cache_ptr_ != NULL);

        lfu_cache_ptr_ = new LFUCachePolicy<Key, Value, KeyHasher>();
        assert(lfu_cache_ptr_ != NULL);
    }
    
    LfuLocalCache::~LfuLocalCache()
    {
        assert(rwlock_for_lfu_local_cache_ptr_ != NULL);
        delete rwlock_for_lfu_local_cache_ptr_;
        rwlock_for_lfu_local_cache_ptr_ = NULL;

        assert(lfu_cache_ptr_ != NULL);
        delete lfu_cache_ptr_;
        lfu_cache_ptr_ = NULL;
    }

    // (1) Check is cached and access validity

    bool LfuLocalCache::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::isLocalCached()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock_shared(context_name);

        bool is_cached = lfu_cache_ptr_->exists(key);

        rwlock_for_lfu_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache

    bool LfuLocalCache::getLocalCache(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::getLocalCache()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lfu_cache_ptr_->get(key, value);

        rwlock_for_lfu_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    bool LfuLocalCache::updateLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LFU cache)
        std::string context_name = "LfuLocalCache::updateLocalCache()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = lfu_cache_ptr_->update(key, value);

        rwlock_for_lfu_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LfuLocalCache::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void LfuLocalCache::admitLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::admitLocalCache()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock(context_name);

        lfu_cache_ptr_->admit(key, value);

        rwlock_for_lfu_local_cache_ptr_->unlock(context_name);
        return;
    }

    bool LfuLocalCache::getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::getLocalCacheVictimKey()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock_shared(context_name);

        bool has_victim_key = lfu_cache_ptr_->getVictimKey(key);

        rwlock_for_lfu_local_cache_ptr_->unlock_shared(context_name);
        return has_victim_key;
    }

    bool LfuLocalCache::evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::evictLocalCacheIfKeyMatch()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock(context_name);

        bool is_evict = lfu_cache_ptr_->evictIfKeyMatch(key, value);

        rwlock_for_lfu_local_cache_ptr_->unlock(context_name);
        return is_evict;
    }

    // (4) Other functions

    uint64_t LfuLocalCache::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "LfuLocalCache::getSizeForCapacity()";
        rwlock_for_lfu_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = lfu_cache_ptr_->getSizeForCapacity();

        rwlock_for_lfu_local_cache_ptr_->unlock_shared(context_name);
        return internal_size;
    }

    void LfuLocalCache::checkPointers_() const
    {
        assert(rwlock_for_lfu_local_cache_ptr_ != NULL);
        assert(lfu_cache_ptr_ != NULL);
    }

}