/*
 * SegcacheLocalCache: local edge cache with SegCache group-level policy (https://github.com/thesys-lab/segcache).
 *
 * NOTE: all configuration and function calls refer to SegCache files, including lib/segcache/benchmarks/thrpt_bench.c, lib/segcache/benchmarks/storage_seg/storage_seg.c, and src/cache/segcache/src/storage/seg/seg.c::seg_get_new() (see hacked versions in src/cache/segcache to convert global variables into thread-local variables for multi-threading).
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#ifndef SEGCACHE_LOCAL_CACHE_H
#define SEGCACHE_LOCAL_CACHE_H

#include <string>

#include "cache/local_cache_base.h"
extern "C" { // Such that compiler will use C naming convention for functions instead of C++
    #include "cache/segcache/src/storage/seg/seg.h" // seg_metrics_st, seg_options_st, SET_OPTION, SEG_METRIC, seg_setup
    #include "cache/segcache/src/storage/seg/segcache.h" // struct SegCache
}

namespace covered
{
    class SegcacheLocalCache : public LocalCacheBase
    {
    public:
        // NOTE: too small cache capacity cannot support segment-based memory allocation in SegCache (see src/cache/segcache/benchmarks/storage_seg/storage_seg.c)
        static const uint64_t SEGCACHE_MIN_CAPACITY_BYTES;

        SegcacheLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~SegcacheLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, Value& value) const override;
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value) override;

        virtual void updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const override; // Triggered by get/put/delrsp for cache miss for admission policy if any

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::set<Key>& keys, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;

        // (5) SegCache-specific functions

        bool appendLocalCache_(const Key& key, const Value& value);

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        unsigned segcache_options_cnt_; // # of options in segcache_options_
        seg_options_st* segcache_options_ptr_; // Options for SegCache
        unsigned segcache_metrics_cnt_; // # of metrics in segcache_metrics_
        seg_metrics_st* segcache_metrics_ptr_; // Metrics for SegCache
        struct SegCache* segcache_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif