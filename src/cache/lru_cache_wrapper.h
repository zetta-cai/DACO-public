/*
 * LruCacheWrapper: the wrapper for LRU cache (https://github.com/lamerman/cpp-lru-cache).
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cache/cpp-lru-cache/lrucache.h"
#include "edge/edge_param.h"

namespace covered
{
    class LruCacheWrapper : public CacheWrapperBase
    {
    public:
        LruCacheWrapper(const uint32_t& capacity, EdgeParam* edge_param_ptr);
        ~LruCacheWrapper();

        virtual bool needIndependentAdmit(const Key& key) override;
        
    private:
        static const std::string kClassName;

        virtual bool getInternal_(const Key& key, Value& value) override;
        virtual bool updateInternal_(const Key& key, const Value& value) override;
        virtual void admitInternal_(const Key& key, const Value& value) override;
        virtual void evictInternal_(Key& key, Value& value) override;

        // In units of bytes
        virtual uint32_t getSizeInternal_() const override;

        // lruCacheWrapper only uses edge index to specify instance_name_, yet not need to check if edge is running due to no network communication -> no need to maintain edge_param_ptr_
        std::string instance_name_;

        LruCache* lru_cache_ptr_;
    };
}