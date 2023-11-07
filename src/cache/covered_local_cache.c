#include "cache/covered_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

#include "common/util.h"

namespace covered
{
    //const uint64_t CoveredLocalCache::CACHELIB_ENGINE_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB

    const std::string CoveredLocalCache::kClassName("CoveredLocalCache");

    CoveredLocalCache::CoveredLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt) : LocalCacheBase(edge_idx), capacity_bytes_(capacity_bytes), peredge_synced_victimcnt_(peredge_synced_victimcnt), local_cached_metadata_(), local_uncached_metadata_(local_uncached_capacity_bytes)
    {
        // (A) Const variable

        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // (B) Non-const shared variables of local cached objects for eviction

        // Prepare cacheConfig for CacheLib part of COVERED local cache (most parameters are default values)
        LruCacheConfig cacheConfig;
        // NOTE: we use default allocationClassSizeFactor and minAllocationClassSize yet modify maxAllocationClassSize (lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h) to set allocSizes in MemoryAllocator.config_ for slab-based memory allocation
        cacheConfig.setDefaultAllocSizes(cacheConfig.allocationClassSizeFactor, CACHELIB_ENGINE_MAX_SLAB_SIZE, cacheConfig.minAllocationClassSize, cacheConfig.reduceFragmentationInAllocationClass);
        max_allocation_class_size_ = cacheConfig.maxAllocationClassSize;
        assert(max_allocation_class_size_ == CACHELIB_ENGINE_MAX_SLAB_SIZE);
        // NOTE: we limit cache capacity outside CoveredLocalCache (in EdgeWrapper); here we set cachelib local cache size as overall cache capacity to avoid cache capacity constraint inside CoveredLocalCache
        uint64_t over_provisioned_capacity_bytes = capacity_bytes + COMMON_ENGINE_INTERNAL_UNUSED_CAPACITY_BYTES; // Just avoid internal eviction yet NOT affect cooperative edge caching
        if (over_provisioned_capacity_bytes >= CACHELIB_ENGINE_MIN_CAPACITY_BYTES)
        {
            cacheConfig.setCacheSize(over_provisioned_capacity_bytes);
        }
        else
        {
            cacheConfig.setCacheSize(CACHELIB_ENGINE_MIN_CAPACITY_BYTES);
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

    bool CoveredLocalCache::getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const
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
            affect_victim_tracker = local_cached_metadata_.updateForExistingKey(key, value, value, false, peredge_synced_victimcnt_);
        }

        // NOTE: for getreq with cache miss, we will update local uncached metadata for getres by updateLocalUncachedMetadataForRspInternal_(key)

        return is_local_cached;
    }

    std::list<VictimCacheinfo> CoveredLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        for (uint32_t least_popular_rank = 0; least_popular_rank < peredge_synced_victimcnt_; least_popular_rank++)
        {
            Key tmp_victim_key;
            Popularity tmp_local_cached_popularity = 0.0;
            Popularity tmp_redirected_cached_popularity = 0.0;
            Reward tmp_local_reward = 0.0;

            ObjectSize tmp_victim_object_size = 0;
            bool is_least_popular_key_exist = local_cached_metadata_.getLeastPopularKeyObjsizePopularity(least_popular_rank, tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);

            if (is_least_popular_key_exist)
            {
                uint32_t tmp_victim_value_size = 0;
                LruCacheReadHandle tmp_victim_handle = covered_cache_ptr_->find(tmp_victim_key.getKeystr()); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
                assert(tmp_victim_handle != nullptr); // Victim must be cached before eviction
                tmp_victim_value_size = tmp_victim_handle->getSize();
                #ifdef TRACK_PERKEY_OBJSIZE
                // NOTE: tmp_victim_object_size got from key-level metadata is already accurate
                assert(tmp_victim_object_size == (tmp_victim_key.getKeyLength() + tmp_victim_value_size));
                #else
                // NOTE: tmp_victim_object_size got from group-level metadata is approximate, which should be replaced with the accurate one from local edge cache
                tmp_victim_object_size = tmp_victim_key.getKeyLength() + tmp_victim_value_size;
                #endif

                VictimCacheinfo tmp_victim_info(tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);
                assert(tmp_victim_info.isComplete()); // NOTE: victim cacheinfos from local edge cache MUST be complete
                local_synced_victim_cacheinfos.push_back(tmp_victim_info); // Add to the tail of the list
            }
            else
            {
                break;
            }
        }

        assert(local_synced_victim_cacheinfos.size() <= peredge_synced_victimcnt_);

        return local_synced_victim_cacheinfos;
    }

    void CoveredLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        ObjectSize object_size = 0;
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = local_uncached_metadata_.getLocalUncachedObjsizePopularityForKey(key, object_size, local_uncached_popularity);

        // NOTE: (value size checking) NOT local-get/remote-collect large-value uncached object for aggregated uncached metadata due to slab-based memory management in Cachelib cache engine
        if ((object_size >= key.getKeyLength()) && ((object_size - key.getKeyLength()) > max_allocation_class_size_)) // May be with large value size
        {
            // NOTE: warn instead of assert, as approximate avg object size includes key size which introduces uncertainty
            std::ostringstream oss;
            oss << "getCollectedPopularityFromLocalCacheInternal_() may get a large-value uncached object for key " << key.getKeystr() << " with avg object size " << object_size << " bytes";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }

        collected_popularity = CollectedPopularity(is_key_tracked, local_uncached_popularity, object_size);

        return;
    }

    bool CoveredLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        const std::string keystr = key.getKeystr();
        is_successful = false;

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
        bool is_local_cached = (handle != nullptr);

        // Check value length
        if ((value.getValuesize() > max_allocation_class_size_))
        {
            is_successful = false; // NOT cache too large value due to slab class size limitation of Cachelib -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
            return is_local_cached;
        }

        if (is_local_cached) // Key already exists
        {
            Value original_value(handle->getSize());

            auto allocate_handle = covered_cache_ptr_->allocate(covered_poolid_, keystr, value.getValuesize());
            if (allocate_handle == nullptr)
            {
                //is_local_cached = false; // (OBSOLETE) cache may fail to evict due to too many pending writes -> equivalent to NOT caching the latest value

                is_successful = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT caching the latest value (BUT still local cached, yet will be invalidated by CacheWrapper later)
            }
            else
            {
                std::string valuestr = value.generateValuestr();
                assert(valuestr.size() == value.getValuesize());
                std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
                covered_cache_ptr_->insertOrReplace(allocate_handle); // Must replace

                // Update local cached metadata for getrsp with invalid hit and put/delreq with cache hit
                affect_victim_tracker = local_cached_metadata_.updateForExistingKey(key, value, original_value, true, peredge_synced_victimcnt_);

                is_successful = true;
            }
        }

        return is_local_cached;
    }

    void CoveredLocalCache::updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const
    {
        assertCapacityForLargeObj_(key, value);

        // NOTE: (value size checking) NOT track large-value uncached object in local uncached metadata due to slab-based memory management in Cachelib cache engine
        const bool is_valid_valuesize = (value.getValuesize() <= max_allocation_class_size_);
        
        bool is_key_tracked = local_uncached_metadata_.isKeyExist(key);
        if (is_key_tracked) // Key is tracked by local uncached metadata
        {
            uint32_t original_value_size = local_uncached_metadata_.getValueSizeForUncachedObjects(key);
            if (is_valid_valuesize) // With valid value size
            {
                local_uncached_metadata_.updateForExistingKey(key, value, Value(original_value_size), is_value_related); // For getrsp with cache miss, put/delrsp with cache miss
            }
            else // Large value size -> NOT track the key in local uncached metadata
            {
                local_uncached_metadata_.removeForExistingKey(key, Value(original_value_size)); // Remove original value size to detrack the key from local uncached metadata
            }
        }
        else // Key is NOT tracked by local uncached metadata
        {
            if (is_valid_valuesize) // With valid value size
            {
                // NOTE: tracking a new key in local uncached metadata may detrack old keys due to local uncached capacity constraint
                local_uncached_metadata_.addForNewKey(key, value); // For getrsp with cache miss, put/delrsp with cache miss
            }
        }
        return;
    }

    // (3) Local edge cache management

    bool CoveredLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(key);
        UNUSED(value);
        
        // COVERED will NEVER invoke this function for independent admission
        return false;
    }

    void CoveredLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        is_successful = false;
        const std::string keystr = key.getKeystr();

        // NOTE: (value size checking) NOT admit large-value uncached object after local/remote placement notification due to slab-based memory management in Cachelib cache engine
        /*// (OBSOLETE due to approximate avg object size tracked in local uncached metadata) NOTE: MUST with a valid value length, as we will never track local uncached metadata, collect for aggregated uncached metadata, and receive local/remote placement notification for large-value uncached objects
        assert(value.getValuesize() <= max_allocation_class_size_);*/
        if (value.getValuesize() > max_allocation_class_size_)
        {
            // NOTE: warn instead of assert, as approximate avg object size includes key size which introduces uncertainty
            std::ostringstream oss;
            oss << "admitLocalCacheInternal_() admits a large-value uncached object for key " << key.getKeystr() << " with value size " << value.getValuesize() << " bytes";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }

        LruCacheReadHandle handle = covered_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        assert(!is_local_cached); // Key should NOT exist

        auto allocate_handle = covered_cache_ptr_->allocate(covered_poolid_, keystr, value.getValuesize());
        if (allocate_handle == nullptr)
        {
            //is_local_cached = false; // (OBSOLETE) cache may fail to evict due to too many pending writes -> equivalent to NOT admitting the new key-value pair

            is_successful = false; // cache may fail to evict due to too many pending writes -> equivalent to NOT admitting the new key-value pair (CacheWrapper will NOT add track validity flag for the object)
        }
        else
        {
            std::string valuestr = value.generateValuestr();
            assert(valuestr.size() == value.getValuesize());
            std::memcpy(allocate_handle->getMemory(), valuestr.data(), value.getValuesize());
            covered_cache_ptr_->insertOrReplace(allocate_handle); // Must insert

            // Update local cached metadata for admission
            local_cached_metadata_.addForNewKey(key, value, affect_victim_tracker);

            // Remove from local uncached metadata if necessary for admission
            if (local_uncached_metadata_.isKeyExist(key))
            {
                // NOTE: get/put/delrsp with cache miss MUST already udpate local uncached metadata with the current value before admission, so we can directly use current value to remove it from local uncached metadata instead of approximate value
                local_uncached_metadata_.removeForExistingKey(key, value);
            }

            is_successful = true;
        }

        return;
    }

    bool CoveredLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        // TODO: this function will be invoked at each candidate neighbor node by the beacon node for lazy fetching of candidate victims
        // TODO: this function will also be invoked at each placement neighbor node by the beacon node for cache placement
        
        assert(hasFineGrainedManagement());
        
        // NOTE: (i) we find victims via COVERED's heterogeneous popularity tracking instead of slab-based eviction in CacheLib engine, so required size of eviction is NOT limited by max slab size; (ii) although we ONLY track and admit popular uncached objects w/ reasonable value sizes (NOT exceed max slab size) due to slab-based memory management in CacheLib engine, required size could still exceed max slab size due to multiple admissions in parallel
        //assert(required_size <= max_allocation_class_size_); // (OBSOLETE) NOTE: required size must <= newly-admited object size, while we will never admit large-value objects

        bool has_victim_key = false;
        uint32_t least_popular_rank = 0;
        uint64_t conservative_victim_total_size = 0;
        while (conservative_victim_total_size < required_size) // Provide multiple victims based on required size for lazy victim fetching
        {
            Key tmp_victim_key;
            Popularity tmp_local_cached_popularity = 0.0;
            Popularity tmp_redirected_cached_popularity = 0.0;
            Reward tmp_local_reward = 0.0;

            //bool is_least_popular_key_exist = local_cached_metadata_.getLeastPopularKey(least_popular_rank, tmp_victim_key);
            ObjectSize tmp_victim_object_size = 0;
            bool is_least_popular_key_exist = local_cached_metadata_.getLeastPopularKeyObjsizePopularity(least_popular_rank, tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);
            if (is_least_popular_key_exist)
            {
                if (keys.find(tmp_victim_key) == keys.end())
                {
                    keys.insert(tmp_victim_key);

                    const std::string tmp_victim_keystr = tmp_victim_key.getKeystr();
                    LruCacheReadHandle tmp_victim_handle = covered_cache_ptr_->find(tmp_victim_keystr); // NOTE: although find() will move the item to the front of the LRU list to update recency information inside cachelib, covered uses local cache metadata tracked outside cachelib for cache management
                    assert(tmp_victim_handle != nullptr); // Victim must be cached before eviction
                    uint32_t tmp_victim_value_size = tmp_victim_handle->getSize();
                    #ifdef TRACK_PERKEY_OBJSIZE
                    // NOTE: tmp_victim_object_size got from key-level metadata is already accurate
                    assert(tmp_victim_object_size == (tmp_victim_key.getKeyLength() + tmp_victim_value_size));
                    #else
                    // NOTE: tmp_victim_object_size got from group-level metadata is approximate, which should be replaced with the accurate one from local edge cache
                    tmp_victim_object_size = tmp_victim_key.getKeyLength() + tmp_victim_value_size;
                    #endif

                    VictimCacheinfo tmp_victim_info(tmp_victim_key, tmp_victim_object_size, tmp_local_cached_popularity, tmp_redirected_cached_popularity, tmp_local_reward);
                    assert(tmp_victim_info.isComplete()); // NOTE: victim cacheinfos from local edge cache MUST be complete
                    victim_cacheinfos.push_back(tmp_victim_info); // Add to the tail of the list

                    // NOTE: we CANNOT use delta of cachelib internal sizes as conservative_victim_total_size (still conservative due to NOT considering saved metadata space), as we have NOT really evicted the victims from covered_cache_ptr_ yet
                    conservative_victim_total_size += (tmp_victim_object_size); // Count key size and value size into victim total size (conservative as we do NOT count metadata cache size usage -> the actual saved space after eviction should be larger than conservative_victim_total_size and also required_size)
                }

                has_victim_key = true;
                least_popular_rank += 1;
            }
            else
            {
                std::ostringstream oss;
                oss << "least_popular_rank " << least_popular_rank << " has used up popularity information for local cached objects -> cannot make room for the required size (" << required_size << " bytes) under cache capacity (" << capacity_bytes_ << " bytes)";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);

                //break;
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
            assert(value.getValuesize() <= max_allocation_class_size_); // NOTE: we will never admit large-value objects into edge cache

            // Remove the corresponding cache item
            // NOTE: although CacheLib does NOT free slab memory immediately after remove, it will reclaim slab memory after all handles to the key are destoryed
            CachelibLruCache::RemoveRes removeRes = covered_cache_ptr_->remove(key.getKeystr());
            assert(removeRes == CachelibLruCache::RemoveRes::kSuccess);

            // (OBSOLETE: we CANNOT explicitly free the slab memory pointed by handle, which will be freed by CacheLib after all handles are destroyed!!!) NOTE: remove() just evicts the victim object from Cachelib yet NOT reclaim its slab memory!!!
            //LruCacheWriteHandle tmp_write_handle = std::move(handle).toWriteHandle(); // NOTE: from now we should NOT use handle, which has been converted to a rvalue reference
            //assert(tmp_write_handle.get() != nullptr);
            //covered_cache_ptr_->allocator_->free(tmp_write_handle.get());

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

    void CoveredLocalCache::assertCapacityForLargeObj_(const Key& key, const Value& value) const
    {
        const uint32_t object_size = key.getKeyLength() + value.getValuesize();
        assert(capacity_bytes_ > object_size);
        return;
    }
}