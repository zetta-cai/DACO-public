#include "cache/cachelib_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

#include "common/util.h"

namespace covered
{
    const uint64_t CachelibLocalCache::CACHELIB_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB

    const std::string CachelibLocalCache::kClassName("CachelibLocalCache");

    CachelibLocalCache::CachelibLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Prepare cacheConfig for Cachelib local cache (most parameters are default values)
        Lru2QCacheConfig cacheConfig;
        // NOTE: we use default allocationClassSizeFactor, maxAllocationClassSize, and minAllocationClassSize (lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h) to set allocSizes in MemoryAllocator.config_ for slab-based memory allocation
        max_allocation_class_size_ = cacheConfig.maxAllocationClassSize;
        // NOTE: we limit cache capacity outside CachelibLocalCache (in EdgeWrapper); here we set cachelib local cache size as overall cache capacity to avoid cache capacity constraint inside CachelibLocalCache
        if (capacity_bytes >= CACHELIB_MIN_CAPACITY_BYTES)
        {
            cacheConfig.setCacheSize(capacity_bytes);
        }
        else
        {
            cacheConfig.setCacheSize(CACHELIB_MIN_CAPACITY_BYTES);
        }
        cacheConfig.validate(); // will throw if bad config

        cachelib_cache_ptr_ = std::make_unique<CachelibLru2QCache>(cacheConfig);
        assert(cachelib_cache_ptr_.get() != NULL);

        cachelib_poolid_ = cachelib_cache_ptr_->addPool("default", cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize);
    }
    
    CachelibLocalCache::~CachelibLocalCache()
    {
        // NOTE: deconstructor of cachelib_cache_ptr_ will delete cachelib_cache_ptr_.get() automatically
    }

    const bool CachelibLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool CachelibLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        const std::string keystr = key.getKeystr();

        // NOTE: NO need to invoke recordAccess() as isLocalCached() does NOT update local metadata

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_cached = (handle != nullptr);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool CachelibLocalCache::getLocalCacheInternal_(const Key& key, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(affect_victim_tracker); // Only for COVERED

        const std::string keystr = key.getKeystr();

        // NOTE: NOT take effect as cacheConfig.nvmAdmissionPolicyFactory is empty by default
        //cachelib_cache_ptr_->recordAccess(keystr);

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        if (is_local_cached)
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());
        }

        return is_local_cached;
    }

    std::list<VictimCacheinfo> CachelibLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void CachelibLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool CachelibLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;
        
        const std::string keystr = key.getKeystr();

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);

        // Check value length
        if ((value.getValuesize() > max_allocation_class_size_))
        {
            is_successful = false; // NOT cache too large value due to slab class size limitation of Cachelib -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
            return is_local_cached;
        }

        if (is_local_cached) // Key already exists
        {
            auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
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
                cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must replace

                is_successful = true;
            }
        }

        return is_local_cached;
    }

    void CachelibLocalCache::updateLocalUncachedMetadataForRspInternal_(const Key& key, const Value& value, const bool& is_value_related) const
    {
        // CacheLib (LRU2Q) cache uses default admission policy (i.e., always admit), which does NOT need to update local metadata for get/putres of uncached objects
        return;
    }

    // (3) Local edge cache management

    bool CachelibLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // CacheLib (LRU2Q) cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        const bool is_valid_valuesize = (value.getValuesize() <= max_allocation_class_size_);
        return !is_local_cached && is_valid_valuesize;
    }

    void CachelibLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // NOTE: MUST with a valid value length, as we always return false in needIndependentAdmitInternal_() if value is too large
        assert(value.getValuesize() <= max_allocation_class_size_);
        
        const std::string keystr = key.getKeystr();

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);
        assert(!is_local_cached); // Key should NOT exist

        auto allocate_handle = cachelib_cache_ptr_->allocate(cachelib_poolid_, keystr, value.getValuesize());
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
            cachelib_cache_ptr_->insertOrReplace(allocate_handle); // Must insert
            
            is_successful = true;
        }

        return;
    }

    bool CachelibLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(victim_cacheinfos); // ONLY for COVERED

        // Get allocation class in our memory allocator
        assert(required_size <= std::numeric_limits<uint32_t>::max());
        assert(required_size <= max_allocation_class_size_);

        // NOTE: we enumerate all slabs (from min-size to max-size slabs), as we may need multiple victims to make room for the required size (CacheServer will use a while loop to evict multiple victims until required size is satisfied)
        const uint32_t one_byte = 1;
        auto cid = cachelib_cache_ptr_->allocator_->getAllocationClassId(cachelib_poolid_, static_cast<uint32_t>(one_byte));
        // (OBSOLETE due to NO victim bug for small/large slab class sizes) OTE: Cachelib performs eviction in the given slab class, so we should use the lower-bound slab class for required size to find a victim instead of using the minimum slab class, which may NOT be able to find victims
        //auto cid = cachelib_cache_ptr_->allocator_->getAllocationClassId(cachelib_poolid_, static_cast<uint32_t>(required_size));
        bool has_victim_key = false;
        while (true)
        {
            const uint32_t tmp_slab_class_size = cachelib_cache_ptr_->allocator_->getAllocSize(cachelib_poolid_, cid);

            // NOTE: Cachelib will directly evict the found victim object in findEviction()!!!
            Lru2QCacheItem* item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, cid);
            if (item_ptr != nullptr) // Find a victim for the given slab class size
            {
                std::string victim_keystr((const char*)item_ptr->getKey().data(), item_ptr->getKey().size()); // data() returns b_, while size() return e_ - b_
                Key tmp_victim_key(victim_keystr);

                if (keys.find(tmp_victim_key) == keys.end())
                {
                    keys.insert(tmp_victim_key);
                }

                // NOTE: we do NOT need to recycle the memory of victim CacheItem, so we free it here (refer to src/cache/cachelib/CacheAllocator-inl.h)
                cachelib_cache_ptr_->allocator_->free(item_ptr); // NOTE: this will decrease currAllocSize_ of corresponding MemoryPool and hence affect cache size usage bytes for capacity
                item_ptr = NULL;

                has_victim_key = true;
                break;
            }
            else // NO victim for given slab class size (e.g., Cachelib will NOT maintain small slabs after sufficient admissions and hence NO victim for small slab class sizes)
            {
                if (tmp_slab_class_size == max_allocation_class_size_) // Already achieve the last slab
                {
                    break;
                }
                else // Move to the next slab
                {
                    cid++;
                }
            }
        }

        return has_victim_key;
    }

    bool CachelibLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        bool is_evict = false;

        // Get victim value
        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(key.getKeystr());

        // (OBSOLETE as we do NOT need to evict the victim again, which has been done in getLocalCacheVictimKeysInternal_() under Cachelib)
        /*if (handle != nullptr) // Key exists
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Remove the corresponding cache item
            CachelibLru2QCache::RemoveRes removeRes = cachelib_cache_ptr_->remove(key.getKeystr());
            assert(removeRes == CachelibLru2QCache::RemoveRes::kSuccess);

            is_evict = true;
        }*/

        // NOTE: Cachelib will directly evict the found victim object in findEviction()!!!
        assert(handle == nullptr);
        is_evict = true;

        return is_evict;
    }

    void CachelibLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    uint64_t CachelibLocalCache::getSizeForCapacityInternal_() const
    {
        // NOTE: should NOT use cachelib_cache_ptr_->getCacheMemoryStats().ramCacheSize, which is usable cache size (i.e. capacity) instead of used size
        uint64_t internal_size = cachelib_cache_ptr_->getUsedSize(cachelib_poolid_);

        return internal_size;
    }

    void CachelibLocalCache::checkPointersInternal_() const
    {
        assert(cachelib_cache_ptr_ != NULL);
        return;
    }

}