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

    // (2) Access local edge cache (KV data and local statistics)

    bool LruLocalCache::getLocalCacheInternal_(const Key& key, Value& value) const
    {
        bool is_local_cached = lru_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    bool LruLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        bool is_local_cached = lru_cache_ptr_->update(key, value);

        return is_local_cached;
    }

    void LruLocalCache::updateLocalUncachedStatisticsForRspInternal_(const Key& key, const Value& value, const bool& is_getrsp) const
    {
        // LRU cache uses default admission policy (i.e., always admit), which does NOT need to update local statistics for get/putres of uncached objects
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

    bool LruLocalCache::getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(admit_key);
        UNUSED(admit_value);

        bool has_victim_key = lru_cache_ptr_->getVictimKey(key);

        return has_victim_key;
    }

    bool LruLocalCache::evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        assert(hasFineGrainedManagement());

        UNUSED(admit_key);
        UNUSED(admit_value);

        bool is_evict = lru_cache_ptr_->evictIfKeyMatch(key, value);

        return is_evict;
    }

    void LruLocalCache::evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheInternal_() is not supported due to fine-grained management");
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