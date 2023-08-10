/*
 * SegcacheLocalCache: local edge cache with SegCache group-level policy (https://github.com/thesys-lab/segcache) (thread safe).
 *
 * NOTE: all non-const shared variables in SegcacheLocalCache should be thread safe.
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#ifndef SEGCACHE_LOCAL_CACHE_H
#define SEGCACHE_LOCAL_CACHE_H

#include <string>

#include "cache/local_cache_base.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class SegcacheLocalCache : public LocalCacheBase
    {
    public:
        SegcacheLocalCache(const uint32_t& edge_idx);
        virtual ~SegcacheLocalCache();

        // (1) Check is cached and access validity

        virtual bool isLocalCached(const Key& key) const override;

        // (2) Access local edge cache

        virtual bool getLocalCache(const Key& key, Value& value) const override;
        virtual bool updateLocalCache(const Key& key, const Value& value) override;

        // (3) Local edge cache management

        virtual bool needIndependentAdmit(const Key& key) const override;
        virtual void admitLocalCache(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const override;
        virtual bool evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacity() const override;
    private:
        static const std::string kClassName;

        // (4) Other functions

        virtual void checkPointers_() const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Guarantee the atomicity of local SegCache cache and local statistics
        mutable Rwlock* rwlock_for_segcache_local_cache_ptr_;

        // Non-const shared variables
        LruCache* lru_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif