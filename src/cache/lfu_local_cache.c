#include "cache/lfu_local_cache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string LfuLocalCache::kClassName("LfuLocalCache");

    LfuLocalCache::LfuLocalCache(const uint32_t& edge_idx) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        lfu_cache_ptr_ = new LFUCachePolicy<Key, Value, KeyHasher>();
        assert(lfu_cache_ptr_ != NULL);
    }
    
    LfuLocalCache::~LfuLocalCache()
    {
        assert(lfu_cache_ptr_ != NULL);
        delete lfu_cache_ptr_;
        lfu_cache_ptr_ = NULL;
    }

    const bool LfuLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool LfuLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = lfu_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool LfuLocalCache::getLocalCacheInternal_(const Key& key, Value& value) const
    {
        bool is_local_cached = lfu_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    bool LfuLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        bool is_local_cached = lfu_cache_ptr_->update(key, value);

        return is_local_cached;
    }

    void LfuLocalCache::updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const
    {
        // LFU cache uses default admission policy (i.e., always admit), which does NOT need to update local metadata for get/putres of uncached objects
        return;
    }

    // (3) Local edge cache management

    bool LfuLocalCache::needIndependentAdmitInternal_(const Key& key) const
    {
        // LFU cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void LfuLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value)
    {
        lfu_cache_ptr_->admit(key, value);

        return;
    }

    bool LfuLocalCache::getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(admit_key);
        UNUSED(admit_value);

        bool has_victim_key = lfu_cache_ptr_->getVictimKey(key);

        return has_victim_key;
    }

    bool LfuLocalCache::evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        assert(hasFineGrainedManagement());

        UNUSED(admit_key);
        UNUSED(admit_value);

        bool is_evict = lfu_cache_ptr_->evictIfKeyMatch(key, value);

        return is_evict;
    }

    void LfuLocalCache::evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    uint64_t LfuLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = lfu_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void LfuLocalCache::checkPointersInternal_() const
    {
        assert(lfu_cache_ptr_ != NULL);
        return;
    }

}