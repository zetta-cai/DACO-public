#include "cache/lrb_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string LrbLocalCache::kClassName("LrbLocalCache");

    LrbLocalCache::LrbLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint32_t& dataset_keycnt) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        lrb_cache_ptr_ = new LRBCache(dataset_keycnt);
        assert(lrb_cache_ptr_ != NULL);

        // NOTE: refer to lib/lrb/src/simulation.cpp to configure LRB cache after initializing the instance
        lrb_cache_ptr_->setSize(capacity_bytes);
        lrb_cache_ptr_->init_with_params(std::map<std::string, std::string>()); // Use default parameter settings
    }
    
    LrbLocalCache::~LrbLocalCache()
    {
        assert(lrb_cache_ptr_ != NULL);
        delete lrb_cache_ptr_;
        lrb_cache_ptr_ = NULL;
    }

    const bool LrbLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool LrbLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = lrb_cache_ptr_->has(key.getKeystr());

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool LrbLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        SimpleRequest req = buildRequest_(key);
        const bool is_update = false;
        bool is_local_cached = lrb_cache_ptr_->access(req, is_update);
        value = Value(req.valuestr.length());

        return is_local_cached;
    }

    bool LrbLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY used by COVERED
        UNUSED(is_global_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = lrb_cache_ptr_->has(key.getKeystr());
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            SimpleRequest req = buildRequest_(key, value);
            const bool is_update = true;
            bool tmp_is_local_cached = lrb_cache_ptr_->access(req, is_update);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LrbLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // LRB cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void LrbLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        bool is_local_cached = isLocalCachedInternal_(key);
        if (!is_local_cached) // NOTE: NOT admit if key exists
        {
            SimpleRequest req = buildRequest_(key, value);
            lrb_cache_ptr_->admit(req);
            is_successful = true;
        }

        return;
    }

    bool LrbLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY used by COVERED

        bool has_victim_key = (lrb_cache_ptr_->getInCacheMetadataCnt() > 0);
        if (has_victim_key)
        {
            std::string tmp_victim_keystr = lrb_cache_ptr_->getVictimKey();
            Key tmp_victim_key(tmp_victim_keystr);

            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool LrbLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        const std::string keystr = key.getKeystr();
        std::string valuestr = "";
        bool is_evict = lrb_cache_ptr_->evict(keystr, valuestr);
        value = Value(valuestr.length());

        return is_evict;
    }

    void LrbLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void LrbLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void LrbLocalCache::invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "invokeConstCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t LrbLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = lrb_cache_ptr_->getCurrentSize();

        return internal_size;
    }

    void LrbLocalCache::checkPointersInternal_() const
    {
        assert(lrb_cache_ptr_ != NULL);
        return;
    }

    bool LrbLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

    SimpleRequest LrbLocalCache::buildRequest_(const Key& key, const Value& value)
    {
        const int64_t objsize = key.getKeyLength() + value.getValuesize();
        SimpleRequest req(0, objsize, true, key.getKeystr(), value.generateValuestrForStorage());

        return req;
    }

}