#include "cache/lru_cache_wrapper.h"

#include <assert.h>
#include <sstream>

namespace covered
{
    const std::string LruCacheWrapper::kClassName("LruCacheWrapper");

    LruCacheWrapper::LruCacheWrapper(const uint32_t& capacity, EdgeParam* edge_param_ptr) : CacheWrapperBase(capacity, edge_param_ptr)
    {
        // Differentiate local edge cache in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_local_statistics_ptr_";
        rwlock_for_local_statistics_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_local_statistics_ptr_ != NULL);

        lru_cache_ptr_ = new LruCache();
        assert(lru_cache_ptr_ != NULL);
    }
    
    LruCacheWrapper::~LruCacheWrapper()
    {
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        delete rwlock_for_local_statistics_ptr_;
        rwlock_for_local_statistics_ptr_ = NULL;

        if (lru_cache_ptr_ != NULL)
        {
            delete lru_cache_ptr_;
            lru_cache_ptr_ = NULL;
        }
    }

    bool LruCacheWrapper::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    bool LruCacheWrapper::getInternal_(const Key& key, Value& value) const
    {
        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_local_statistics_ptr_->try_lock("getInternal_()"))
            {
                break;
            }
        }

        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->get(key, value);

        rwlock_for_local_statistics_ptr_->unlock();
        return is_local_cached;
    }

    bool LruCacheWrapper::updateInternal_(const Key& key, const Value& value)
    {
        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_local_statistics_ptr_->try_lock("updateInternal_()"))
            {
                break;
            }
        }

        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->update(key, value);

        rwlock_for_local_statistics_ptr_->unlock();
        return is_local_cached;
    }

    void LruCacheWrapper::admitInternal_(const Key& key, const Value& value)
    {
        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_local_statistics_ptr_->try_lock("admitInternal_()"))
            {
                break;
            }
        }

        assert(lru_cache_ptr_ != NULL);
        lru_cache_ptr_->admit(key, value);

        rwlock_for_local_statistics_ptr_->unlock();
        return;
    }

    void LruCacheWrapper::evictInternal_(Key& key, Value& value)
    {
        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_local_statistics_ptr_->try_lock("evictInternal_()"))
            {
                break;
            }
        }

        assert(lru_cache_ptr_ != NULL);
        lru_cache_ptr_->evict(key, value);

        rwlock_for_local_statistics_ptr_->unlock();
        return;
    }

    uint32_t LruCacheWrapper::getSizeInternal_() const
    {
        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        assert(rwlock_for_local_statistics_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_local_statistics_ptr_->try_lock_shared("getSizeInternal_()"))
            {
                break;
            }
        }

        assert(lru_cache_ptr_ != NULL);
        uint32_t internal_size = lru_cache_ptr_->getSize();

        rwlock_for_local_statistics_ptr_->unlock_shared();
        return internal_size;
    }

}