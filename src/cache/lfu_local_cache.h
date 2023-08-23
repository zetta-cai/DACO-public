/*
 * LfuLocalCache: local edge cache with LFU policy (https://github.com/vpetrigo/caches.git).
 * 
 * By Siyuan Sheng (2023.08.07).
 */

#ifndef LFU_LOCAL_CACHE_H
#define LFU_LOCAL_CACHE_H

#include <string>

#include "cache/lfu/lfu_cache_policy.hpp"
#include "cache/local_cache_base.h"

namespace covered
{
    class LfuLocalCache : public LocalCacheBase
    {
    public:
        LfuLocalCache(const uint32_t& edge_idx);
        virtual ~LfuLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related) const override; // Triggered by get/put/delrsp for cache miss for admission policy if any

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
        LFUCachePolicy<Key, Value, KeyHasher>* lfu_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif