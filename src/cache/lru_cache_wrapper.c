#include "cache/lru_cache_wrapper.h"

#include <assert.h>

namespace covered
{
    const std::string LruCacheWrapper::kClassName("LruCacheWrapper");

    LruCacheWrapper::LruCacheWrapper(const uint32_t& capacity) : CacheWrapperBase(capacity), lru_cache_ptr_(new LruCache())
    {
        assert(lru_cache_ptr_ != NULL);
    }
    
    LruCacheWrapper::~LruCacheWrapper()
    {
        if (lru_cache_ptr_ != NULL)
        {
            delete lru_cache_ptr_;
            lru_cache_ptr_ = NULL;
        }
    }

    bool LruCacheWrapper::get(const Key& key, Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->get(key, value);
        return is_local_cached;
    }

    bool LruCacheWrapper::update(const Key& key, const Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->update(key, value);
        return is_local_cached;
    }

    bool LruCacheWrapper::needIndependentAdmit(const Key& key)
    {
        // LRU cache uses LRU-based independent admission policy (i.e., always admit) which always returns true
        return true;
    }

    void LruCacheWrapper::admitInternal_(const Key& key, const Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        lru_cache_ptr_->admit(key, value);
        return;
    }

    Key LruCacheWrapper::evictInternal_()
    {
        assert(lru_cache_ptr_ != NULL);
        Key victim_key = lru_cache_ptr_->evict();
        return victim_key;
    }

    uint32_t LruCacheWrapper::getSizeInternal_() const
    {
        assert(lru_cache_ptr_ != NULL);
        uint32_t internal_size = lru_cache_ptr_->getSize();
        return internal_size;
    }

}