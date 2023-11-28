#include "cache/cachelib_local_cache.h"

#include <assert.h>
#include <sstream>

#include <cachelib/cachebench/util/CacheConfig.h>
#include <folly/init/Init.h>

#include "common/util.h"

namespace covered
{
    //const uint64_t CachelibLocalCache::CACHELIB_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB

    const std::string CachelibLocalCache::kClassName("CachelibLocalCache");

    CachelibLocalCache::CachelibLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Prepare cacheConfig for Cachelib local cache (most parameters are default values)
        Lru2QCacheConfig cacheConfig;
        // NOTE: we use default allocationClassSizeFactor and minAllocationClassSize yet modify maxAllocationClassSize (lib/CacheLib/cachelib/allocator/CacheAllocatorConfig.h) to set allocSizes in MemoryAllocator.config_ for slab-based memory allocation
        cacheConfig.setDefaultAllocSizes(cacheConfig.allocationClassSizeFactor, CACHELIB_ENGINE_MAX_SLAB_SIZE, cacheConfig.minAllocationClassSize, cacheConfig.reduceFragmentationInAllocationClass);
        max_allocation_class_size_ = cacheConfig.maxAllocationClassSize;
        assert(max_allocation_class_size_ == CACHELIB_ENGINE_MAX_SLAB_SIZE);
        // NOTE: we limit cache capacity outside CachelibLocalCache (in EdgeWrapper); here we set cachelib local cache size as overall cache capacity to avoid cache capacity constraint inside CachelibLocalCache
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
        //return true; // (OBSOLETE) Key-level (i.e., object-level) cache management

        return false; // Slab-level cache management
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

    bool CachelibLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
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

    bool CachelibLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_getrsp); // Only for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;
        
        const std::string keystr = key.getKeystr();

        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(keystr);
        bool is_local_cached = (handle != nullptr);

        // Check value length
        if ((key.getKeyLength() + value.getValuesize()) > max_allocation_class_size_)
        {
            is_successful = false; // NOT cache too large object size due to slab class size limitation of Cachelib -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
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

    // (3) Local edge cache management

    bool CachelibLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // CacheLib (LRU2Q) cache uses default admission policy (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        const bool is_valid_valuesize = ((key.getKeyLength() + value.getValuesize()) <= max_allocation_class_size_);
        return !is_local_cached && is_valid_valuesize;
    }

    void CachelibLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // NOTE: MUST with a valid value length, as we always return false in needIndependentAdmitInternal_() if object size is too large
        assert((key.getKeyLength() + value.getValuesize()) <= max_allocation_class_size_);
        
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

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeysInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool CachelibLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheWithGivenKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;

        // OBSOLETE: as we do NOT need to evict the victim again, which has been done in getLocalCacheVictimKeysInternal_() under Cachelib

        /*assert(hasFineGrainedManagement());

        bool is_evict = false;

        // Get victim value
        Lru2QCacheReadHandle handle = cachelib_cache_ptr_->find(key.getKeystr());
        if (handle != nullptr) // Key exists
        {
            //std::string value_string{reinterpret_cast<const char*>(handle->getMemory()), handle->getSize()};
            value = Value(handle->getSize());

            // Remove the corresponding cache item
            // NOTE: although CacheLib does NOT free slab memory immediately after remove, it will reclaim slab memory after all handles to the key are destoryed
            CachelibLru2QCache::RemoveRes removeRes = cachelib_cache_ptr_->remove(key.getKeystr());
            assert(removeRes == CachelibLru2QCache::RemoveRes::kSuccess);

            //// (OBSOLETE: we CANNOT explicitly free the slab memory pointed by handle, which will be freed by CacheLib after all handles are destroyed!!!) NOTE: remove() just evicts the victim object from Cachelib yet NOT reclaim its slab memory!!!
            ////LruCacheWriteHandle tmp_write_handle = std::move(handle).toWriteHandle(); // NOTE: from now we should NOT use handle, which has been converted to a rvalue reference
            ////assert(tmp_write_handle.get() != nullptr);
            ////covered_cache_ptr_->allocator_->free(tmp_write_handle.get());

            is_evict = true;
        }

        return is_evict;*/
    }

    void CachelibLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        UNUSED(required_size); // CacheServer will use a while loop to evict multiple victims until required size is satisfied

        // Get allocation class in our memory allocator
        assert(required_size <= std::numeric_limits<uint32_t>::max());
        
        // NOTE: although we ONLY trigger independent admission for uncached objects w/ reasonable value sizes (NOT exceed max slab size) due to slab-based memory management in CacheLib engine, required size could still exceed max slab size due to multiple admissions in parallel
        //assert(required_size <= max_allocation_class_size_); // (OBSOLETE) NOTE: required size must <= newly-admited object size, while we will never admit large-value objects
        uint64_t tmp_required_size = required_size;
        if (required_size > max_allocation_class_size_)
        {
            // NOTE: extra bytes will be evicted outside CachelibLocalCache in the while loop of CacheServer
            tmp_required_size = max_allocation_class_size_;
        }

        // NOTE: Cachelib performs slab-based eviction, so we use the lower-bound slab class for required size to try to find a victim first
        const auto lower_bound_cid = cachelib_cache_ptr_->allocator_->getAllocationClassId(cachelib_poolid_, static_cast<uint32_t>(tmp_required_size));
        assert(lower_bound_cid >= 0);
        Lru2QCacheItem* item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, lower_bound_cid); // NOTE: Cachelib will directly evict the found victim object in findEviction()!!!

        // NOTE: under limited cache capacity, the lower-bound class NOT have any cached objects and hence cannot find any victims -> find victims from nearby slab classes isntead
        if (item_ptr == nullptr) // NO single victim can be found under the lower-bound slab class
        {
            const uint32_t lower_bound_slab_class_size = cachelib_cache_ptr_->allocator_->getAllocSize(cachelib_poolid_, lower_bound_cid);

            if (lower_bound_slab_class_size < max_allocation_class_size_) // Subsequent slab classes exist (i.e., lower-bound slab is NOT the last slab)
            {
                // First enumuerate all subsequent slab classes to find a victim for the required size
                auto tmp_subsequent_cid = lower_bound_cid + 1;
                while (true)
                {
                    item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, tmp_subsequent_cid); // NOTE: Cachelib will directly evict the found victim object in findEviction()!!!
                    if (item_ptr != nullptr) // Find a victim for the given slab class size
                    {
                        break;
                    }
                    else // NO victim for the given slab class size (e.g., NO cached objects in the slab class under limited cache capacity)
                    {
                        const uint32_t tmp_slab_class_size = cachelib_cache_ptr_->allocator_->getAllocSize(cachelib_poolid_, tmp_subsequent_cid);
                        if (tmp_slab_class_size == max_allocation_class_size_) // Already achieve the last slab
                        {
                            break;
                        }
                        else // Move to the next slab
                        {
                            tmp_subsequent_cid++;
                        }
                    }
                }
            }

            if (item_ptr == nullptr && lower_bound_cid >= 1) // NO single victim can be found under all subsequent slab classes, and previous slab classes exist (i.e., lower-bound slab is NOT the first slab)
            {
                // Then enumerate all previous slab classes to find a victim for the required size
                auto tmp_previous_cid = lower_bound_cid - 1;
                while (tmp_previous_cid >= 0)
                {
                    const uint32_t tmp_slab_class_size = cachelib_cache_ptr_->allocator_->getAllocSize(cachelib_poolid_, tmp_previous_cid);
                    assert(tmp_slab_class_size < max_allocation_class_size_); // Prev slab MUST NOT be the last slab

                    item_ptr = cachelib_cache_ptr_->findEviction(cachelib_poolid_, tmp_previous_cid); // NOTE: Cachelib will directly evict the found victim object in findEviction()!!!
                    if (item_ptr != nullptr) // Find a victim for the given slab class size
                    {
                        break;
                    }
                    else // NO victim for the given slab class size (e.g., NO cached objects in the slab class under limited cache capacity)
                    {
                        tmp_previous_cid--; // Move to the prev slab
                    }
                }
            }

            // NOTE: we can finally find multiple victims to make room for the required size (rely on the while loop outside CachelibLocalCache in CacheServer)
        }
        assert(item_ptr != nullptr); // Must find at least one victim from a slab

        // Insert into victims
        std::string victim_keystr((const char*)item_ptr->getKey().data(), item_ptr->getKey().size()); // data() returns b_, while size() return e_ - b_
        Key tmp_victim_key(victim_keystr);
        Value tmp_victim_value(item_ptr->getSize());
        victims.insert(std::pair(tmp_victim_key, tmp_victim_value));

        // NOTE: findEviction() just evicts the victim object from Cachelib yet NOT reclaim its slab memory!!!
        // NOTE: we do NOT need to recycle the memory of victim CacheItem, so we free it here (refer to src/cache/cachelib/CacheAllocator-inl.h)
        cachelib_cache_ptr_->allocator_->free(item_ptr); // NOTE: this will decrease currAllocSize_ of corresponding MemoryPool and hence affect cache size usage bytes for capacity
        item_ptr = NULL;

        return;
    }

    // (4) Other functions

    void CachelibLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, void* func_param_ptr) override
    {
        Util::dumpErrorMsg(instance_name_, "updateLocalCacheMetadataInternal_() is ONLY for COVERED
        !");
        exit(1);
        return;
    }

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