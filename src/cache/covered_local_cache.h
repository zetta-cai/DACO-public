/*
 * CoveredLocalCache: local edge cache built on TommyDS (https://github.com/amadvance/tommyds/tree/master/tommyds).
 *
 * NOTE: NOT use CacheLib engine, as the system bottleneck of cooperative edge caching is network propagation latency instead of single-node cache access, and slab-based memory allocation used in CacheLib is NOT memory efficient due to internal fragmentation.
 * 
 * NOTE: NOT use Memcached/Redis due to slow string comparison and client-server storage architecture (w/ extra cross-thread communication overhead), while TommyDS is a high-performance hashtable-based KVS library with custom object comparison.
 *
 * By Siyuan Sheng (2023.12.04).
 */

#ifndef COVERED_LOCAL_CACHE_H
#define COVERED_LOCAL_CACHE_H

#include <list> // std::list
#include <string>

#include <tommy.h> // TommyDS

namespace covered
{
    // Forward declaration
    class CoveredLocalCache;
}

#include "common/covered_common_header.h"
#include "cache/covered/local_cached_metadata.h"
#include "cache/covered/local_uncached_metadata.h"
#include "cache/local_cache_base.h"

namespace covered
{
    // For internal TommyDS

    typedef struct TommydsObject {
        Key key;
        Value val;
        tommy_node node;
    } tommyds_object_t;

    static int tommyds_compare(const void *arg, const void *obj) {
        const Key *targetkey = (const Key *)arg;
        const tommyds_object_t *obj_to_compare = (const tommyds_object_t *)obj;
        return *targetkey != obj_to_compare->key;
    }

    class CoveredLocalCache : public LocalCacheBase
    {
    public:
        // For updateLocalCacheMetadataInternal_()
        static const std::string UPDATE_IS_NEIGHBOR_CACHED_FLAG_FUNC_NAME; // Update is_neighbor_cached flag in local cached metadata (func param is bool)

        CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt);
        virtual ~CoveredLocalCache();

        virtual const bool hasFineGrainedManagement() const;
    private:
        static const std::string kClassName;

        // (1) Check is cached and access validity

        virtual bool isLocalCachedInternal_(const Key& key) const override;

        // (2) Access local edge cache (KV data and local metadata)

        virtual bool getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const override;
        virtual std::list<VictimCacheinfo> getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const override; // Return up to peredge_synced_victimcnt local synced victims with the least local rewards
        virtual void getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const override; // Return true if local uncached key is tracked (for piggybacking-based popularity colleciton)

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
        
        uint32_t hashForTommyds_(const Key& key) const;

        // Member variables

        // (A) Const variable
        std::string instance_name_;
        const uint32_t peredge_synced_victimcnt_;

        // (B) Non-const shared variables of local cached objects for eviction

        // TommyDS-based key-value storage
        uint64_t internal_kvbytes_; // Internal bytes used for key-value pairs stored by TommyDS (in units of bytes)
        tommy_hashdyn* covered_cache_ptr_; // Data engine for local edge cache (use TommyDS dynamic chained hashtable due to high performance)

        mutable LocalCachedMetadata local_cached_metadata_; // Metadata for local cached objects

        // (C) Non-const shared variables of local uncached objects for admission

        mutable LocalUncachedMetadata local_uncached_metadata_; // Metadata for local uncached objects
    };
}

#endif