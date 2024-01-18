/*
 * FrozenhotLocalCache: local edge cache with FrozenHot policy (refer to https://github.com/USTCqzy/FrozenHot).
 *
 * NOTE: refer to lib/frozenhot/traceloader/client.h to initialize and access frozenhot cache.
 * 
 * By Siyuan Sheng (2024.01.16).
 */

#ifndef FROZENHOT_LOCAL_CACHE_H
#define FROZENHOT_LOCAL_CACHE_H

#include <string>
#include <thread> // std::thread
#include <unordered_set>

#include "cache/frozenhot/hhvm-scalable-cache.h"
#include "cache/local_cache_base.h"
#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class FrozenhotLocalCache : public LocalCacheBase
    {
    public:
        FrozenhotLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~FrozenhotLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        typedef ConcurrentScalableCache<Key, Value, KeyHasher> FrozenhotCache;

        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) override; // Return if key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        virtual void invokeCustomFunctionInternal_(const std::string& func_name, CustomFuncParamBase* func_param_ptr) override; // Invoke some method-specific function for local edge cache

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        void threadInitIfNot_() const; // Initialize CLHT GC if not for the current CPU core ID

        virtual void checkPointersInternal_() const override;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        mutable FrozenhotCache* frozenhot_cache_ptr_; // Data and metadata for local edge cache
        mutable std::unordered_set<uint32_t> thread_init_set_; // Track which CPU cores have been initialized for CLHT GC
        std::thread* monitor_thread_ptr_; // Monitor thread for FrozenHot period construction
    };
}

#endif