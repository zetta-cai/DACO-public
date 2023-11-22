#include "cache/gdsize_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    // TODO: END HERE

    const std::string LruLocalCache::kClassName("LruLocalCache");

    LruLocalCache::LruLocalCache(const uint32_t& edge_idx) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        lru_cache_ptr_ = new LruCache();
        assert(lru_cache_ptr_ != NULL);
    }
    
    LruLocalCache::~LruLocalCache()
    {
        assert(lru_cache_ptr_ != NULL);
        delete lru_cache_ptr_;
        lru_cache_ptr_ = NULL;
    }

    const bool LruLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool LruLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = lru_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool LruLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = lru_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    std::list<VictimCacheinfo> LruLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void LruLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool LruLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;
        
        bool is_local_cached = lru_cache_ptr_->update(key, value);
        if (is_local_cached)
        {
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LruLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // LRU cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void LruLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(affect_victim_tracker); // Only for COVERED
        
        lru_cache_ptr_->admit(key, value);
        is_successful = true;

        return;
    }

    bool LruLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY for COVERED

        Key tmp_victim_key;
        bool has_victim_key = lru_cache_ptr_->getVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool LruLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        bool is_evict = lru_cache_ptr_->evictWithGivenKey(key, value);

        return is_evict;
    }

    void LruLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    uint64_t LruLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = lru_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void LruLocalCache::checkPointersInternal_() const
    {
        assert(lru_cache_ptr_ != NULL);
        return;
    }

}