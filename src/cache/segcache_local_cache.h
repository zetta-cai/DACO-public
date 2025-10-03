/*
 * SegcacheLocalCache: local edge cache with SegCache group-level policy (https://github.com/thesys-lab/segcache).
 *
 * NOTE: all configuration and function calls refer to SegCache files, including lib/segcache/benchmarks/thrpt_bench.c, lib/segcache/benchmarks/storage_seg/storage_seg.c, and src/cache/segcache/src/storage/seg/seg.c::seg_get_new() (see hacked versions in src/cache/segcache to convert global variables into thread-local variables for multi-threading).
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#ifndef SEGCACHE_LOCAL_CACHE_H
#define SEGCACHE_LOCAL_CACHE_H

// (OBSOLETE as segcache uses absolute bytecnt instead of relative objcnt for segment size and merge-based eviction -> always leak 1MiB segsize (cannot be changed; hardcoded in segcache with 20-bit offsets) no matter storing value size or content) NOTE: we store value size instead of value content in SegCache to avoid memory usage bug of SegCache itself, yet still use real value size to calculate cache usage for capacity limitation -> NOT affect cache stable performance.
//#define ENABLE_ONLY_VALSIZE_FOR_SEGCACHE

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
        SegcacheLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~SegcacheLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;
        virtual bool getLocalCacheInternal_p2p_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker, const uint32_t redirected_reward) const override;
        
        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) override; // Return if key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful, const uint64_t& miss_latency_us = 0) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        virtual void invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) override; // Invoke some method-specific function for local edge cache
        virtual void invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const override; // Invoke some method-specific function for local edge cache

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const override;

        // (5) SegCache-specific functions

        // Update cached object for writes or insert for admission
        bool appendLocalCache_(const Key& key, const Value& value, const bool& is_insert, bool& is_successful); // Return whether key is local cached (is_successful indicates whether value is updated/inserted successfully)

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        unsigned segcache_options_cnt_; // # of options in segcache_options_
        seg_options_st* segcache_options_ptr_; // Options for SegCache
        unsigned segcache_metrics_cnt_; // # of metrics in segcache_metrics_
        seg_metrics_st* segcache_metrics_ptr_; // Metrics for SegCache
        struct SegCache* segcache_cache_ptr_; // Data and metadata for local edge cache

        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // NOTE: store value size instead of value content to avoid memory usage bug of segcache, yet not affect cache stable performance, as we use extra_size_bytes_ to complete the remaining bytes which should stored in segcache
        uint32_t extra_size_bytes_; // The remaining bytes which should stored in segcache if we store value content instead of value size
        // NOTE: cannot change 1MiB segment size, as segcache hardcodes to use 20 bits for segment offset
        //const uint32_t logical_seg_size_; // Used for object size checking
        //const uint32_t physical_seg_size_; // Scales in logical seg size into physical one for impl trick of only storing value size instead of value content (otherwise using 1MiB to store value size could cache too many objects, which does not have enough segments to merge when cache is full, and may evict too many objects after evicting a segment)
        #endif
    };
}

#endif