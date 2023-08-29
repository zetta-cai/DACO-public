#include "cache/lru_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
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

    bool LruLocalCache::getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = lru_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    std::list<VictimInfo> LruLocalCache::getLocalSyncedVictimInfosFromLocalCacheInternal_() const
    {
        std::list<VictimInfo> local_synced_victim_infos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimInfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_infos;
    }

    bool LruLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        bool is_local_cached = lru_cache_ptr_->update(key, value);

        return is_local_cached;
    }

    void LruLocalCache::updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const
    {
        // LRU cache uses default admission policy (i.e., always admit), which does NOT need to update local metadata for get/putres of uncached objects
        return;
    }

    // (3) Local edge cache management

    bool LruLocalCache::needIndependentAdmitInternal_(const Key& key) const
    {
        // LRU cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void LruLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value)
    {
        lru_cache_ptr_->admit(key, value);

        return;
    }

    bool LruLocalCache::getLocalCacheVictimKeysInternal_(std::set<Key>& keys, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size);

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