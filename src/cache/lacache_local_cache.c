#include "cache/lacache_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string LacacheLocalCache::kClassName("LacacheLocalCache");

    LacacheLocalCache::LacacheLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        lacache_cache_ptr_ = new LACache();
        assert(lacache_cache_ptr_ != NULL);
    }
    
    LacacheLocalCache::~LacacheLocalCache()
    {
        assert(lacache_cache_ptr_ != NULL);
        delete lacache_cache_ptr_;
        lacache_cache_ptr_ = NULL;
    }

    const bool LacacheLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool LacacheLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = lacache_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool LacacheLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        bool is_local_cached = lacache_cache_ptr_->get(key, value);

        return is_local_cached;
    }
    bool LacacheLocalCache::getLocalCacheInternal_p2p_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker, const uint32_t redirected_reward) const {
        
        std::ostringstream oss;
        oss << "getLocalCacheInternal_p2p_() is NOT supported by " << kClassName << "!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return false;
    }
    bool LacacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY used by COVERED
        UNUSED(is_global_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = lacache_cache_ptr_->exists(key);

        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }
            
            // Update with the latest value
            bool tmp_is_local_cached = lacache_cache_ptr_->update(key, value);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LacacheLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // LA-Cache cache admits only if the lambda value > 0 (i.e., non-first arrival access)
        bool need_admit = lacache_cache_ptr_->needAdmit(key); // Must be locally uncached if need_admit = true
        return need_admit;
    }

    void LacacheLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful, const uint64_t& miss_latency_us)
    {
        UNUSED(is_neighbor_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        // NOTE: ONLY BestGuess and COVERED will use cache server placement processor for admission and hence no miss latency, while LA-Cache will only perform independent admission and hence positive miss latency
        assert(miss_latency_us > 0);
        
        lacache_cache_ptr_->admit(key, value, miss_latency_us);
        is_successful = true;

        return;
    }

    bool LacacheLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY used by COVERED

        Key tmp_victim_key;
        bool has_victim_key = lacache_cache_ptr_->getVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool LacacheLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        bool is_evict = lacache_cache_ptr_->evictWithGivenKey(key, value);

        return is_evict;
    }

    void LacacheLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void LacacheLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void LacacheLocalCache::invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "invokeConstCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t LacacheLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = lacache_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void LacacheLocalCache::checkPointersInternal_() const
    {
        assert(lacache_cache_ptr_ != NULL);
        return;
    }

    bool LacacheLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

}