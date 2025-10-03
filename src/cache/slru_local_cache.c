#include "cache/slru_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string SlruLocalCache::kClassName("SlruLocalCache");

    SlruLocalCache::SlruLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        slru_cache_ptr_ = new SlruCachePolicy<Key, Value, KeyHasher>(capacity_bytes);
        assert(slru_cache_ptr_ != NULL);
    }
    
    SlruLocalCache::~SlruLocalCache()
    {
        assert(slru_cache_ptr_ != NULL);
        delete slru_cache_ptr_;
        slru_cache_ptr_ = NULL;
    }

    const bool SlruLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool SlruLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = slru_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool SlruLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        bool is_local_cached = slru_cache_ptr_->get(key, value);

        return is_local_cached;
    }
    bool SlruLocalCache::getLocalCacheInternal_p2p_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker, const uint32_t redirected_reward) const {
        
        std::ostringstream oss;
        oss << "getLocalCacheInternal_p2p_() is NOT supported by " << kClassName << "!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return false;
    }
    bool SlruLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY used by COVERED
        UNUSED(is_global_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = slru_cache_ptr_->exists(key);
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            bool tmp_is_local_cached = slru_cache_ptr_->update(key, value);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool SlruLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // SLRU cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void SlruLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful, const uint64_t& miss_latency_us)
    {
        UNUSED(is_neighbor_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        UNUSED(miss_latency_us); // ONLY used by LA-Cache
        
        slru_cache_ptr_->admit(key, value);
        is_successful = true;

        return;
    }

    bool SlruLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY used by COVERED

        Key tmp_victim_key;
        bool has_victim_key = slru_cache_ptr_->getVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool SlruLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        bool is_evict = slru_cache_ptr_->evictWithGivenKey(key, value);

        return is_evict;
    }

    void SlruLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void SlruLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void SlruLocalCache::invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "invokeConstCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t SlruLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = slru_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void SlruLocalCache::checkPointersInternal_() const
    {
        assert(slru_cache_ptr_ != NULL);
        return;
    }

    bool SlruLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: SLRU requires objsize <= per LRU size
        const bool is_valid_objsize = slru_cache_ptr_->canAdmit(objsize);
        return is_valid_objsize;
    }

}