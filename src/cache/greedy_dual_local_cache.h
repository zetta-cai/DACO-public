/*
 * GreedyDualLocalCache: local edge cache with greedy dual policy including LRU-K, GD-Size, GDSF, and LFU-DA (https://github.com/dasebe/webcachesim).
 * 
 * By Siyuan Sheng (2023.11.22).
 */

#ifndef GREEDY_DUAL_LOCAL_CACHE_H
#define GREEDY_DUAL_LOCAL_CACHE_H

#include <string>

#include "cache/local_cache_base.h"
#include "cache/greedydual/greedy_dual_base.h"

namespace covered
{
    class GreedyDualLocalCache : public LocalCacheBase
    {
    public:
        GreedyDualLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~GreedyDualLocalCache();

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

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        GreedyDualBase* greedy_dual_cache_ptr_; // Data and metadata for local edge cache
    };
}

#endif