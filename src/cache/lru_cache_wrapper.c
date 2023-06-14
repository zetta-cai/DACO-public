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
        oss << kClassName << " " << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();

        lru_cache_ptr_ = new LruCache();
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

    bool LruCacheWrapper::needIndependentAdmit(const Key& key)
    {
        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    bool LruCacheWrapper::getInternal_(const Key& key, Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->get(key, value);
        return is_local_cached;
    }

    bool LruCacheWrapper::updateInternal_(const Key& key, const Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        bool is_local_cached = lru_cache_ptr_->update(key, value);
        return is_local_cached;
    }

    void LruCacheWrapper::admitInternal_(const Key& key, const Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        lru_cache_ptr_->admit(key, value);
        return;
    }

    void LruCacheWrapper::evictInternal_(Key& key, Value& value)
    {
        assert(lru_cache_ptr_ != NULL);
        lru_cache_ptr_->evict(key, value);
        return;
    }

    uint32_t LruCacheWrapper::getSizeInternal_() const
    {
        assert(lru_cache_ptr_ != NULL);
        uint32_t internal_size = lru_cache_ptr_->getSize();
        return internal_size;
    }

}