/*
 * LruLocalCache: local edge cache with LRU policy (https://github.com/lamerman/cpp-lru-cache).
 * 
 * By Siyuan Sheng (2023.05.16).
 */

#ifndef LRU_LOCAL_CACHE_H
#define LRU_LOCAL_CACHE_H

#include <string>

#include "cache/local_cache_base.h"
#include "cache/lru/lrucache.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class LruLocalCache : public LocalCacheBase
    {
    public:
        LruLocalCache(const uint32_t& edge_idx);
        virtual ~LruLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const override;
        virtual bool evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value) override;
        virtual void evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        LruCache* lru_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif