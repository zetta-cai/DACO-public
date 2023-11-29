#include "cache/greedy_dual_local_cache.h"

#include <assert.h>
#include <sstream>

#include "cache/greedydual/gdsf_cache.h"
#include "cache/greedydual/gdsize_cache.h"
#include "cache/greedydual/lfuda_cache.h"
#include "cache/greedydual/lruk_cache.h"
#include "common/util.h"

namespace covered
{
    const std::string GreedyDualLocalCache::kClassName("GreedyDualLocalCache");

    GreedyDualLocalCache::GreedyDualLocalCache(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        uint64_t over_provisioned_capacity_bytes = capacity_bytes + COMMON_ENGINE_INTERNAL_UNUSED_CAPACITY_BYTES; // Just avoid internal eviction yet NOT affect cooperative edge caching
        if (cache_name == Util::GDSF_CACHE_NAME)
        {
            greedy_dual_cache_ptr_ = new GDSFCache(over_provisioned_capacity_bytes);
        }
        else if (cache_name == Util::GDSIZE_CACHE_NAME)
        {
            greedy_dual_cache_ptr_ = new GDSizeCache(over_provisioned_capacity_bytes);
        }
        else if (cache_name == Util::LFUDA_CACHE_NAME)
        {
            greedy_dual_cache_ptr_ = new LfuDACache(over_provisioned_capacity_bytes);
        }
        else if (cache_name == Util::LRUK_CACHE_NAME)
        {
            greedy_dual_cache_ptr_ = new LRUKCache(over_provisioned_capacity_bytes);
        }
        else
        {
            oss.clear();
            oss.str("");
            oss << "invalid cache name " << cache_name << " for GreedyDualLocalCache";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(greedy_dual_cache_ptr_ != NULL);
    }
    
    GreedyDualLocalCache::~GreedyDualLocalCache()
    {
        assert(greedy_dual_cache_ptr_ != NULL);
        delete greedy_dual_cache_ptr_;
        greedy_dual_cache_ptr_ = NULL;
    }

    const bool GreedyDualLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool GreedyDualLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = greedy_dual_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool GreedyDualLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = greedy_dual_cache_ptr_->lookup(key, value);

        return is_local_cached;
    }

    std::list<VictimCacheinfo> GreedyDualLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void GreedyDualLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool GreedyDualLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;
        
        bool is_local_cached = greedy_dual_cache_ptr_->update(key, value);
        if (is_local_cached)
        {
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool GreedyDualLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // GreedyDual cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void GreedyDualLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        
        greedy_dual_cache_ptr_->admit(key, value);
        is_successful = true;

        return;
    }

    bool GreedyDualLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY for COVERED

        Key tmp_victim_key;
        bool has_victim_key = greedy_dual_cache_ptr_->getVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool GreedyDualLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        bool is_evict = greedy_dual_cache_ptr_->evict(key, value);

        return is_evict;
    }

    void GreedyDualLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void GreedyDualLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, const void* func_param_ptr)
    {
        Util::dumpErrorMsg(instance_name_, "updateLocalCacheMetadataInternal_() is ONLY for COVERED!");
        exit(1);
        return;
    }

    uint64_t GreedyDualLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = greedy_dual_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void GreedyDualLocalCache::checkPointersInternal_() const
    {
        assert(greedy_dual_cache_ptr_ != NULL);
        return;
    }

}