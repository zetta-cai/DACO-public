/*
 * LruCacheWrapper: the wrapper for LRU cache (https://github.com/lamerman/cpp-lru-cache) (thread safe).
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cache/cpp-lru-cache/lrucache.h"
#include "edge/edge_param.h"
#include "lock/rwlock.h"

namespace covered
{
    class LruCacheWrapper : public CacheWrapperBase
    {
    public:
        LruCacheWrapper(const uint32_t& capacity, EdgeParam* edge_param_ptr);
        ~LruCacheWrapper();

        virtual bool isLocalCached(const Key& key) const override;

        virtual bool needIndependentAdmit(const Key& key) const override;
    private:
        static const std::string kClassName;

        virtual bool getInternal_(const Key& key, Value& value) const override;
        virtual bool updateInternal_(const Key& key, const Value& value) override;
        virtual void admitInternal_(const Key& key, const Value& value) override;
        virtual void evictInternal_(Key& key, Value& value) override;

        // In units of bytes
        virtual uint32_t getSizeInternal_() const override;

        // lruCacheWrapper only uses edge index to specify instance_name_, yet not need to check if edge is running due to no network communication -> no need to maintain edge_param_ptr_
        std::string instance_name_;

        // Guarantee the atomicity of local statistics (e.g., get values for different keys)
        mutable Rwlock* rwlock_for_local_statistics_ptr_;

        // Non-const shared variables
        LruCache* lru_cache_ptr_; // Data and metadata for local edge cache
    };
}