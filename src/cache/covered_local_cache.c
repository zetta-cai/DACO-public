#include "cache/covered_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

#include "common/util.h"

namespace covered
{
    const uint64_t CoveredLocalCache::COVERED_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB

    const std::string CoveredLocalCache::kClassName("CoveredLocalCache");

    CoveredLocalCache::CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx), local_cached_metadata_(false, 0), local_uncached_metadata_(true, capacity_bytes*COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO >= COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_BYTES ? COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_BYTES : capacity_bytes*COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO)
    {
        // (A) Const variable

        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // (B) Non-const shared variables of local cached objects for eviction

        // Prepare cacheConfig for CacheLib part of COVERED local cache (most parameters are default values)
        LruCacheConfig cacheConfig;
        // NOTE: we limit cache capacity outside CoveredLocalCache (in EdgeWrapper); here we set cachelib local cache size as overall cache capacity to avoid cache capacity constraint inside CoveredLocalCache
        if (capacity_bytes >= COVERED_MIN_CAPACITY_BYTES)
        {
            cacheConfig.setCacheSize(capacity_bytes);
        }
        else
        {
            cacheConfig.setCacheSize(COVERED_MIN_CAPACITY_BYTES);
        }
        cacheConfig.validate(); // will throw if bad config

        // CacheLib-based key-value storage
        covered_cache_ptr_ = std::make_unique<CachelibLruCache>(cacheConfig);
        assert(covered_cache_ptr_.get() != NULL);
        covered_poolid_ = covered_cache_ptr_->addPool("default", covered_cache_ptr_->getCacheMemoryStats().ramCacheSize);
    }

    CoveredLocalCache::~CoveredLocalCache()
    {
        // NOTE: deconstructor of covered_cache_ptr_ will delete covered_cache_ptr_.get() automatically
    }

    const bool CoveredLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool CoveredLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        const std::string keystr = key.getKeystr();

        // NOTE: NO need to invoke recordAccess() as isLocalCached() does NOT update local metadata

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
        bool is_cached = (handle != nullptr);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool CoveredLocalCache::getLocalCacheInternal_(const Key& key, Value& value) const
    {
        const std::string keystr = key.getKeystr();

        // NOTE: NOT take effect as cacheConfig.nvmAdmissionPolicyFactory is empty by default
        //covered_cache_ptr_->recordAccess(keystr);

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached)
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Update local cached metadata for getreq with cache hit
            local_cached_metadata_.updateForExistingKey(key, value, value, false);
        }

        // NOTE: for getreq with cache miss, we will update local uncached metadata for getres by updateLocalUncachedMetadataForRspInternal_(key)

        return is_local_cached;
    }

    ool CoveredLocalCache::getLocalCacheVictimInfoIfAnyInternal_(const Key& key, VictimInfo& cur_vicim_info, uint32_t& cur_victim_rank) const
    {
        bool is_victim = false;
        
        // TODO: END HERE

        return is_victim;
    }

    bool CoveredLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        const std::string keystr = key.getKeystr();

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached) // Key already exists
        {
            Value original_value(handle->getSize());

            auto allocate_handle = covered_cache_ptr_->allocate(covered_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT caching the latest value
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                covered_cache_ptr_->insertOrReplace(allocate_handle); // Must replace

                // Update local cached metadata for getrsp with invalid hit and put/delreq with cache hit
                local_cached_metadata_.updateForExistingKey(key, value, original_value, true);
            }
        }

        return is_local_cached;
    }

    void CoveredLocalCache::updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const
    {
        bool is_key_tracked = local_uncached_metadata_.isKeyExist(key);
        if (is_key_tracked)
        {
            uint32_t approx_original_value_size = local_uncached_metadata_.getApproxValueForUncachedObjects(key);
            local_uncached_metadata_.updateForExistingKey(key, value, Value(approx_original_value_size), is_value_related); // For getrsp with cache miss, put/delrsp with cache miss
        }
        else
        {
            local_uncached_metadata_.addForNewKey(key, value); // For getrsp with cache miss, put/delrsp with cache miss

            Key detracked_key;
            while (true)
            {
                bool need_detrack = local_uncached_metadata_.needDetrackForUncachedObjects(detracked_key);
                if (need_detrack) // Cache size usage for local uncached objects exceeds the max bytes limitation
                {
                    uint32_t approx_detracked_value_size = local_uncached_metadata_.getApproxValueForUncachedObjects(detracked_key);
                    local_uncached_metadata_.removeForExistingKey(detracked_key, Value(approx_detracked_value_size)); // For getrsp with cache miss, put/delrsp with cache miss
                }
                else // Local uncached objects is limited
                {
                    break;
                }
            }
        }
        return;
    }

    // (3) Local edge cache management

    bool CoveredLocalCache::needIndependentAdmitInternal_(const Key& key) const
    {
        // COVERED will NEVER invoke this function for independent admission
        return false;
    }

    void CoveredLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value)
    {
        const std::string keystr = key.getKeystr();

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (!is_local_cached) // Key does NOT exist
        {
            auto allocate_handle = covered_cache_ptr_->allocate(covered_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT admitting the new key-value pair
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                covered_cache_ptr_->insertOrReplace(allocate_handle); // Must insert

                // Update local cached metadata for admission
                local_cached_metadata_.addForNewKey(key, value);

                // Remove from local uncached metadata if necessary for admission
                if (local_uncached_metadata_.isKeyExist(key))
                {
                    // NOTE: get/put/delrsp with cache miss MUST already udpate local uncached metadata with the current value before admission, so we can directly use current value to remove it from local uncached metadata instead of approximate value
                    local_uncached_metadata_.removeForExistingKey(key, value);
                }
            }
        }

        return;
    }

    bool CoveredLocalCache::getLocalCacheVictimKeysInternal_(std::set<Key>& keys, const uint64_t& required_size) const
    {
        // TODO: this function will be invoked at each candidate neighbor node by the beacon node for lazy fetching of candidate victims
        // TODO: this function will also be invoked at each placement neighbor node by the beacon node for cache placement
        
        assert(hasFineGrainedManagement());

        bool has_victim_key = false;
        uint32_t least_popular_rank = 0;
        uint64_t conservative_victim_total_size = 0;
        while (conservative_victim_total_size < required_size)
        {
            Key tmp_victim_key;
            bool is_least_popular_key_exist = local_cached_metadata_.getLeastPopularKey(least_popular_rank, tmp_victim_key);
            if (is_least_popular_key_exist)
            {
                if (keys.find(tmp_victim_key) == keys.end())
                {
                    keys.insert(tmp_victim_key);

                    const std::string tmp_victim_keystr = tmp_victim_key.getKeystr();
                    LruCacheReadHandle tmp_victim_handle = covered_cache_ptr_->find(tmp_victim_keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
                    assert(tmp_victim_handle != nullptr); // Victim must be cached before eviction
                    uint32_t tmp_victim_value_size = tmp_victim_handle->getSize();

                    conservative_victim_total_size += (tmp_victim_key.getKeystr().length() + tmp_victim_value_size); // Count key size and value size into victim total size (conservative as we do NOT count metadata cache size usage -> the actual saved space after eviction should be larger than conservative_victim_total_size and also required_size)
                }

                has_victim_key = true;
                least_popular_rank += 1;
            }
            else
            {
                std::ostringstream oss;
                oss << "least_popular_rank " << least_popular_rank << " has used up popularity information for local cached objects!";
                Util::dumpWarnMsg(instance_name_, oss.str());

                break;
            }
        }

        return has_victim_key;
    }

    bool CoveredLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        // TODO: this function will be invoked at each placement neighbor node by the beacon node for cache placement

        assert(hasFineGrainedManagement());

        bool is_evict = false;

        // Get victim value
        LruCacheReadHandle handle = covered_cache_ptr_->find(key.getKeystr());
        if (handle != nullptr) // Key exists
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Remove the corresponding cache item
            CachelibLruCache::RemoveRes removeRes = covered_cache_ptr_->remove(key.getKeystr());
            assert(removeRes == CachelibLruCache::RemoveRes::kSuccess);

            // Remove from local cached metadata for eviction
            local_cached_metadata_.removeForExistingKey(key, value);

            is_evict = true;   
        }

        return is_evict;
    }

    void CoveredLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    uint64_t CoveredLocalCache::getSizeForCapacityInternal_() const
    {
        // NOTE: should NOT use cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize, which is usable cache size (i.e. capacity) instead of used size
        uint64_t internal_size = covered_cache_ptr_->getUsedSize(covered_poolid_);

        // Count cache size usage for local cached objects
        uint64_t local_cached_metadata_size = local_cached_metadata_.getSizeForCapacity();
        internal_size = Util::uint64Add(internal_size, local_cached_metadata_size);

        // Count cache size usage for local uncached objects
        uint64_t local_uncached_metadata_size = local_uncached_metadata_.getSizeForCapacity();
        internal_size = Util::uint64Add(internal_size, local_uncached_metadata_size);

        return internal_size;
    }

    void CoveredLocalCache::checkPointersInternal_() const
    {
        assert(covered_cache_ptr_ != NULL);
        return;
    }
}