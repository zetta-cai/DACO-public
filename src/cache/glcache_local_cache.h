/*
 * GLCacheLocalCache: local edge cache with GLCache policy (https://github.com/Thesys-lab/fast23-glcache).
 *
 * NOTE: all configuration and function calls refer to GLCache files, including lib/glcache/micro-implementation/test/test_glcache.c and lib/glcache/micro-implementation/libCacheSim/cache/eviction/GLCache/*, yet reimplement in C++ (see src/cache/glcache/micro-implementation/*) and fix libcachesim limitations (only metadata operations + fixed-length uint64_t key + impractical assumption of in-request next access time).
 *
 * Hack to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * NOTE: GLCache is a coarse-grained cache replacement policy due to segment-level merge-based eviction.
 * 
 * By Siyuan Sheng (2024.01.05).
 */

#ifndef GLCACHE_LOCAL_CACHE_H
#define GLCACHE_LOCAL_CACHE_H

#include <string>

#include <libCacheSim/cache.h> // src/cache/glcache/micro-implementation/build/include/libCacheSim/cache.h

#include "cache/local_cache_base.h"

namespace covered
{
    class GLCacheLocalCache : public LocalCacheBase
    {
    public:
        GLCacheLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes);
        virtual ~GLCacheLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;
        virtual std::list<VictimCacheinfo> getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const override; // Return up to peredge_synced_victimcnt local synced victims with the least local rewards
        virtual void getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const override; // Return true if local uncached key is tracked

        virtual bool updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful) override; // Return if key is local cached for getrsp/put/delreq (is_getrsp indicates getrsp w/ invalid hit or cache miss; is_successful indicates whether value is updated successfully)

        // (3) Local edge cache management

        virtual bool needIndependentAdmitInternal_(const Key& key, const Value& value) const override;
        virtual void admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful) override;
        virtual bool getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const override;
        virtual bool evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value) override;
        virtual void evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) override;

        // (4) Other functions

        virtual void updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, const void* func_param_ptr) override; // Update local metadata (e.g., is_neighbor_cached) for local edge cache

        // In units of bytes
        virtual uint64_t getSizeForCapacityInternal_() const override;

        virtual void checkPointersInternal_() const override;
        virtual bool checkObjsizeInternal_(const ObjectSize& objsize) const override;

        // Member variables

        // Const variable
        std::string instance_name_;

        // Non-const shared variables
        cache_t* glcache_ptr_; // Data and metadata for local edge cache
    };
}

#endif