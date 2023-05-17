/*
 * LruCacheWrapper: the wrapper for LRU cache (https://github.com/lamerman/cpp-lru-cache).
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cache/cpp-lru-cache/lrucache.h"

namespace covered
{
    class LruCacheWrapper : public CacheWrapperBase
    {
    public:
        LruCacheWrapper(const uint32_t& capacity);
        ~LruCacheWrapper();

        virtual bool get(const Key& key, Value& value) override;
        virtual bool update(const Key& key, const Value& value) override;

        virtual bool needIndependentAdmit() override;
        
    private:
        static const std::string kClassName;

        virtual void admitInternal_(const Key& key, const Value& value) override;
        virtual Key evictInternal_() override;

        // In units of bytes
        virtual uint32_t getSizeInternal_() const override;

        LruCache* const lru_cache_ptr_;
    };
}