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

    bool LfuLocalCache::getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = lfu_cache_ptr_->get(key, value);

        return is_local_cached;
    }

    std::list<VictimCacheinfo> LfuLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void LfuLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool LfuLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker)
    {
        UNUSED(affect_victim_tracker); // Only for COVERED
        
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
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void LfuLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker)
    {
        UNUSED(affect_victim_tracker); // Only for COVERED
        
        lfu_cache_ptr_->admit(key, value);

        return;
    }

    bool LfuLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY for COVERED

        Key tmp_victim_key;
        bool has_victim_key = lfu_cache_ptr_->getVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool LfuLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        bool is_evict = lfu_cache_ptr_->evictWithGivenKey(key, value);

        return is_evict;
    }

    void LfuLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
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