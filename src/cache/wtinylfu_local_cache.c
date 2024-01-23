#include "cache/wtinylfu_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string WTinylfuLocalCache::kClassName("WTinylfuLocalCache");

    WTinylfuLocalCache::WTinylfuLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        wtinylfu_cache_ptr_ = new WTinylfuCachePolicy<Key, Value, KeyHasher>(capacity_bytes);
        assert(wtinylfu_cache_ptr_ != NULL);
    }
    
    WTinylfuLocalCache::~WTinylfuLocalCache()
    {
        assert(wtinylfu_cache_ptr_ != NULL);
        delete wtinylfu_cache_ptr_;
        wtinylfu_cache_ptr_ = NULL;
    }

    const bool WTinylfuLocalCache::hasFineGrainedManagement() const
    {
        return false; // CANNOT find and evict a given victim due to from-window-to-main eviction in W-TinyLFU (see src/cache/wtinylfu/wtinylfu_cache_policy.hpp for details)
    }

    // (1) Check is cached and access validity

    bool WTinylfuLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = wtinylfu_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool WTinylfuLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = wtinylfu_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    bool WTinylfuLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = wtinylfu_cache_ptr_->exists(key);
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            bool tmp_is_local_cached = wtinylfu_cache_ptr_->update(key, value);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool WTinylfuLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // S3-FIFO cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void WTinylfuLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // ONLY for COVERED
        
        wtinylfu_cache_ptr_->admit(key, value);
        is_successful = true;

        return;
    }

    bool WTinylfuLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeysInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool WTinylfuLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheWithGivenKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    void WTinylfuLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        UNUSED(required_size);

        wtinylfu_cache_ptr_->evictNoGivenKey(victims);

        return;
    }

    // (4) Other functions

    void WTinylfuLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t WTinylfuLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = wtinylfu_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void WTinylfuLocalCache::checkPointersInternal_() const
    {
        assert(wtinylfu_cache_ptr_ != NULL);
        return;
    }

    bool WTinylfuLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: W-TinyLFU requires objsize <= window cache size and main cache size
        const bool is_valid_objsize = wtinylfu_cache_ptr_->canAdmit(objsize);
        return is_valid_objsize;
    }

}