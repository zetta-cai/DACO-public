#include "cache/covered_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

#include "common/util.h"

namespace covered
{
    const uint64_t CoveredLocalCache::COVERED_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB
    const uint32_t CoveredLocalCache::COVERED_PERGROUP_MAXKEYCNT = 10; // At most 10 keys per group for local cached/uncached objects
    const uint32_t CoveredLocalCached::COVERED_LOCALCACHED_MAXKEYCNT = 1000; // At most 1000 keys in total for local uncached objects

    const std::string CoveredLocalCache::kClassName("CoveredLocalCache");

    CoveredLocalCache::CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
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
        covered_cache_ptr_ = std::make_unique<LruCache>(cacheConfig);
        assert(covered_cache_ptr_.get() != NULL);
        covered_poolid_ = covered_cache_ptr_->addPool("default", covered_cache_ptr_->getCacheMemoryStats().ramCacheSize);

        // Local cached object-level statistics
        local_cached_perkey_statistics_.clear();

        // Local cached group-level statistics
        local_cached_cur_group_id_ = 0;
        local_cached_cur_group_keycnt_ = 0;
        local_cached_pergroup_statistics_.clear();

        // Local cached popularity information
        local_cached_sorted_popularity_.clear();

        // (C) Non-const shared variables of local uncached objects for admission

        // Local uncached object-level statistics
        local_uncached_perkey_statistics_.clear();

        // Local uncached group-level statistics
        local_uncached_cur_group_id_ = 0;
        local_uncached_cur_group_keycnt_ = 0;
        local_uncached_pergroup_statistics_.clear();

        // Local uncached popularity information
        local_uncached_sorted_popularity_.clear();
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

        // NOTE: NO need to invoke recordAccess() as isLocalCached() does NOT update local statistics

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr);
        bool is_cached = (handle != nullptr);

        return is_cached;
    }

    // (2) Access local edge cache

    bool CoveredLocalCache::getLocalCacheInternal_(const Key& key, Value& value) const
    {
        const std::string keystr = key.getKeystr();

        // NOTE: NOT take effect as cacheConfig.nvmAdmissionPolicyFactory is empty by default
        //covered_cache_ptr_->recordAccess(keystr);

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr); // NOTE: find() will move the item to the front of the LRU list to update recency information
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached)
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // TODO: Update local cached statistics
        }
        else
        {
            // TODO: Update local uncached statistics
        }

        return is_local_cached;
    }

    bool CoveredLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        const std::string keystr = key.getKeystr();

        LruCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached) // Key already exists
        {
            auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT caching the latest value
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must replace
            }
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool CoveredLocalCache::needIndependentAdmitInternal_(const Key& key) const
    {
        // CacheLib (LRU2Q) cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void CoveredLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value)
    {
        const std::string keystr = key.getKeystr();

        LruCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (!is_local_cached) // Key does NOT exist
        {
            auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                is_local_cached = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT admitting the new key-value pair
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must insert
            }
        }

        return;
    }

    bool CoveredLocalCache::getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        assert(hasFineGrainedManagement());

        const std::string admit_keystr = admit_key.getKeystr();

        // number of bytes required for this item
        const auto requiredSize = LruCacheItem::getRequiredSize(admit_keystr, admit_value.getValuesize());
        // TODO: maybe we can set requiredSize as 1 byte to ensure that we can always find a victim key

        // the allocation class in our memory allocator.
        const auto cid = cachelib_cache_ptr_->allocator_->getAllocationClassId(cachelib_poolid_, requiredSize);

        LruCacheItem* item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, cid);
        bool has_victim_key = (item_ptr != nullptr);
        if (has_victim_key)
        {
            std::string victim_keystr((const char*)item_ptr->getKey().data(), item_ptr->getKey().size()); // data() returns b_, while size() return e_ - b_
            key = Key(victim_keystr);
        }

        return has_victim_key;
    }

    bool CoveredLocalCache::evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        assert(hasFineGrainedManagement());

        bool is_evict = false;

        // Select victim by LFU for version check
        Key cur_victim_key;
        bool has_victim_key = getLocalCacheVictimKey(cur_victim_key, admit_key, admit_value);
        if (has_victim_key && cur_victim_key == key) // Key matches
        {
            // Get victim value
            LruCacheReadHandle handle = cachelib_cache_ptr_->find(cur_victim_key.getKeystr());
            assert(handle != nullptr);
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Remove the corresponding cache item
            LruCache::RemoveRes removeRes = cachelib_cache_ptr_->remove(cur_victim_key.getKeystr());
            assert(removeRes == LruCache::RemoveRes::kSuccess);

            is_evict = true;
        }

        return is_evict;
    }

    void CoveredLocalCache::evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    uint64_t CoveredLocalCache::getSizeForCapacityInternal_() const
    {
        // NOTE: should NOT use cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize, which is usable cache size (i.e. capacity) instead of used size
        uint64_t internal_size = cachelib_cache_ptr_->getUsedSize(cachelib_poolid_);

        return internal_size;
    }

    void CoveredLocalCache::checkPointersInternal_() const
    {
        assert(cachelib_cache_ptr_ != NULL);
        return;
    }

    // (5) COVERED-specific functions

    // Grouping

    uint32_t CoveredLocalCache::assignGroupIdForAdmission_(const Key& key)
    {
        assert(local_cached_perkey_statistics_.find(key) == local_cached_perkey_statistics_.end()); // key must NOT be admitted before

        local_cached_cur_group_keycnt_++;
        if (local_cached_cur_group_keycnt_ > COVERED_PERGROUP_MAXKEYCNT)
        {
            local_cached_cur_group_id_++;
            local_cached_cur_group_keycnt_ = 1;
        }     

        return local_cached_cur_group_id_;
    }
    
    uint32_t CoveredLocalCache::getGroupIdForLocalCachedKey_(const Key& key) const
    {
        std::unordered_map<Key, LocalCachedPerkeyStatistics, KeyHasher>::const_iterator iter = local_cached_perkey_statistics_.find(key);
        assert(iter != local_cached_perkey_statistics_.end()); // key must be admitted before

        return iter->second.getGroupId();
    }

    // Update statistics
    
    void CoveredLocalCache::updateLocalCachedStatistics_(const Key& key)
    {
        // Update local cached object-level statistics
        std::unordered_map<Key, LocalCachedPerkeyStatistics, KeyHasher>::iterator iter = local_cached_perkey_statistics_.find(key);
        assert(iter != local_cached_perkey_statistics_.end());
        iter->second.update();

        // Update local cached group-level statistics
        uint32_t tmp_group_id = iter->second.getGroupId();

        return;
    }

}