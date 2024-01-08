#include "cache/s3fifo_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string S3fifoLocalCache::kClassName("S3fifoLocalCache");

    S3fifoLocalCache::S3fifoLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        s3fifo_cache_ptr_ = new S3fifoCachePolicy<Key, Value, KeyHasher>(capacity_bytes);
        assert(s3fifo_cache_ptr_ != NULL);
    }
    
    S3fifoLocalCache::~S3fifoLocalCache()
    {
        assert(s3fifo_cache_ptr_ != NULL);
        delete s3fifo_cache_ptr_;
        s3fifo_cache_ptr_ = NULL;
    }

    const bool S3fifoLocalCache::hasFineGrainedManagement() const
    {
        return false; // CANNOT find and evict a given victim due to recursive eviction between S3-FIFO small/main cache (see src/cache/s3fifo/s3fifo_cache_policy.hpp for details)
    }

    // (1) Check is cached and access validity

    bool S3fifoLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = s3fifo_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool S3fifoLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = s3fifo_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    std::list<VictimCacheinfo> S3fifoLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void S3fifoLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool S3fifoLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = s3fifo_cache_ptr_->exists(key);
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            bool tmp_is_local_cached = s3fifo_cache_ptr_->update(key, value);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool S3fifoLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // S3-FIFO cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void S3fifoLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // ONLY for COVERED
        
        s3fifo_cache_ptr_->admit(key, value);
        is_successful = true;

        return;
    }

    bool S3fifoLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeysInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool S3fifoLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheWithGivenKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    void S3fifoLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        UNUSED(required_size);

        s3fifo_cache_ptr_->evictNoGivenKey(victims);

        return;
    }

    // (4) Other functions

    void S3fifoLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, const void* func_param_ptr)
    {
        Util::dumpErrorMsg(instance_name_, "updateLocalCacheMetadataInternal_() is ONLY for COVERED!");
        exit(1);
        return;
    }

    uint64_t S3fifoLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = s3fifo_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void S3fifoLocalCache::checkPointersInternal_() const
    {
        assert(s3fifo_cache_ptr_ != NULL);
        return;
    }

    bool S3fifoLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

}