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
#include "cache/segcache/src/storage/seg/seg.h" // seg_metrics_st, seg_options_st, SET_OPTION, SEG_METRIC, seg_setup
#include "cache/segcache/src/storage/seg/segcache.h" // struct SegCache
#include "concurrency/rwlock.h"

namespace covered
{
    class SegcacheLocalCache : public LocalCacheBase
    {
    public:
        // NOTE: too small cache capacity cannot support segment-based memory allocation in SegCache (see src/cache/segcache/benchmarks/storage_seg/storage_seg.c)
        static const uint64_t SEGCACHE_MIN_CAPACITY_BYTES;

        SegcacheLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes);
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
        unsigned segcache_options_cnt_; // # of options in segcache_options_
        seg_options_st* segcache_options_ptr_; // Options for SegCache
        unsigned segcache_metrics_cnt_; // # of metrics in segcache_metrics_
        seg_metrics_st* segcache_metrics_ptr_; // Metrics for SegCache
        struct SegCache* segcache_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif