#include "cache/cachelib_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

namespace covered
{
    const std::string CachelibLocalCache::kClassName("CachelibLocalCache");

    CachelibLocalCache::CachelibLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_cachelib_local_cache_ptr_";
        rwlock_for_cachelib_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_cachelib_local_cache_ptr_ != NULL);

        // Prepare cacheConfig for Cachelib local cache (most parameters are default values)
        Lru2QCacheConfig cacheConfig;
        cacheConfig.setCacheSize(capacity_bytes); // NOTE: we limit cache capacity outside CachelibLocalCache (in EdgeWrapper); here we set cachelib local cache size as overall cache capacity to avoid cache capacity constraint inside CachelibLocalCache
        cacheConfig.validate(); // will throw if bad config

        cachelib_cache_ptr_ = std::make_unique<Lru2QCache>(cacheConfig);
        assert(cachelib_cache_ptr_.get() != NULL);

        cachelib_poolid_ = cachelib_cache_ptr_->addPool("default", cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize);
    }
    
    CachelibLocalCache::~CachelibLocalCache()
    {
        assert(rwlock_for_cachelib_local_cache_ptr_ != NULL);
        delete rwlock_for_cachelib_local_cache_ptr_;
        rwlock_for_cachelib_local_cache_ptr_ = NULL;

        // NOTE: deconstructor of cachelib_cache_ptr_ will delete cachelib_cache_ptr_.get() automatically
    }

    // (1) Check is cached and access validity

    bool CachelibLocalCache::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::isLocalCached()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock_shared(context_name);

        const std::string keystr = key.getKeystr();

        // NOTE: NO need to invoke recordAccess() as isLocalCached() does NOT update local statistics

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_cached = (handle != nullptr);

        rwlock_for_cachelib_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache

    bool CachelibLocalCache::getLocalCache(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::getLocalCache()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock(context_name);

        const std::string keystr = key.getKeystr();

        // NOTE: NOT take effect as cacheConfig.nvmAdmissionPolicyFactory is empty by default
        //cachelib_cache_ptr_->recordAccess(keystr);

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached)
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());
        }

        rwlock_for_cachelib_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    bool CachelibLocalCache::updateLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LFU cache)
        std::string context_name = "CachelibLocalCache::updateLocalCache()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock(context_name);

        const std::string keystr = key.getKeystr();

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached) // Key already exists
        {
            auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT caching the latest value
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must replace
            }
        }

        rwlock_for_cachelib_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool CachelibLocalCache::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void CachelibLocalCache::admitLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::admitLocalCache()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock(context_name);

        const std::string keystr = key.getKeystr();

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (!is_local_cached) // Key does NOT exist
        {
            auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT admitting the new key-value pair
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must insert
            }
        }

        rwlock_for_cachelib_local_cache_ptr_->unlock(context_name);
        return;
    }

    bool CachelibLocalCache::getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::getLocalCacheVictimKey()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock_shared(context_name);

        const std::string admit_keystr = admit_key.getKeystr();

        // number of bytes required for this item
        const auto requiredSize = Lru2QCacheItem::getRequiredSize(admit_keystr, admit_value.getValuesize());
        // TODO: maybe we can set requiredSize as 1 byte to ensure that we can always find a victim key

        // the allocation class in our memory allocator.
        const auto cid = cachelib_cache_ptr_->allocator_->getAllocationClassId(cachelib_poolid_, requiredSize);

        Lru2QCacheItem* item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, cid);
        bool has_victim_key = (item_ptr != nullptr);
        if (has_victim_key)
        {
            std::string victim_keystr((const char*)item_ptr->getKey().data(), item_ptr->getKey().size()); // data() returns b_, while size() return e_ - b_
            key = Key(victim_keystr);
        }

        rwlock_for_cachelib_local_cache_ptr_->unlock_shared(context_name);
        return has_victim_key;
    }

    bool CachelibLocalCache::evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::evictLocalCacheIfKeyMatch()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock(context_name);

        bool is_evict = false;

        // Select victim by LFU for version check
        Key cur_victim_key;
        bool has_victim_key = getLocalCacheVictimKey(cur_victim_key, admit_key, admit_value);
        if (has_victim_key && cur_victim_key == key) // Key matches
        {
            // Get victim value
            Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(cur_victim_key.getKeystr());
            assert(handle != nullptr);
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Remove the corresponding cache item
            Lru2QCache::RemoveRes removeRes = cachelib_cache_ptr_->remove(cur_victim_key.getKeystr());
            assert(removeRes == Lru2QCache::RemoveRes::kSuccess);

            is_evict = true;
        }

        rwlock_for_cachelib_local_cache_ptr_->unlock(context_name);
        return is_evict;
    }

    // (4) Other functions

    uint64_t CachelibLocalCache::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "CachelibLocalCache::getSizeForCapacity()";
        rwlock_for_cachelib_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize;

        rwlock_for_cachelib_local_cache_ptr_->unlock_shared(context_name);
        return internal_size;
    }

    void CachelibLocalCache::checkPointers_() const
    {
        assert(rwlock_for_cachelib_local_cache_ptr_ != NULL);
        assert(cachelib_cache_ptr_ != NULL);
    }

}