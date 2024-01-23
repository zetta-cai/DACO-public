#include "cache/local_cache_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "cache/arc_local_cache.h"
#include "cache/cachelib_local_cache.h"
#include "cache/covered_local_cache.h"
#include "cache/fifo_local_cache.h"
#include "cache/frozenhot_local_cache.h"
#include "cache/lfu_local_cache.h"
#include "cache/lhd_local_cache.h"
#include "cache/lrb_local_cache.h"
#include "cache/lru_local_cache.h"
#include "cache/glcache_local_cache.h"
#include "cache/greedy_dual_local_cache.h"
#include "cache/s3fifo_local_cache.h"
#include "cache/segcache_local_cache.h"
#include "cache/sieve_local_cache.h"
#include "cache/slru_local_cache.h"
#include "cache/wtinylfu_local_cache.h"

namespace covered
{
    const std::string LocalCacheBase::kClassName("LocalCacheBase");

    LocalCacheBase* LocalCacheBase::getLocalCacheByCacheName(const EdgeWrapper* edge_wrapper_ptr, const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt)
    {
        LocalCacheBase* local_cache_ptr = NULL;
        if (cache_name == Util::ARC_CACHE_NAME)
        {
            local_cache_ptr = new ArcLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::CACHELIB_CACHE_NAME)
        {
            local_cache_ptr = new CachelibLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::FIFO_CACHE_NAME)
        {
            local_cache_ptr = new FifoLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::FROZENHOT_CACHE_NAME)
        {
            local_cache_ptr = new FrozenhotLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::GLCACHE_CACHE_NAME)
        {
            local_cache_ptr = new GLCacheLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LRUK_CACHE_NAME || cache_name == Util::GDSIZE_CACHE_NAME || cache_name == Util::GDSF_CACHE_NAME || cache_name == Util::LFUDA_CACHE_NAME) // Greedy dual
        {
            local_cache_ptr = new GreedyDualLocalCache(edge_wrapper_ptr, cache_name, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LFU_CACHE_NAME)
        {
            local_cache_ptr = new LfuLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LHD_CACHE_NAME)
        {
            local_cache_ptr = new LhdLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LRB_CACHE_NAME)
        {
            local_cache_ptr = new LrbLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LRU_CACHE_NAME)
        {
            local_cache_ptr = new LruLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::S3FIFO_CACHE_NAME)
        {
            local_cache_ptr = new S3fifoLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::SEGCACHE_CACHE_NAME)
        {
            local_cache_ptr = new SegcacheLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::SIEVE_CACHE_NAME)
        {
            local_cache_ptr = new SieveLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::SLRU_CACHE_NAME)
        {
            local_cache_ptr = new SlruLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::WTINYLFU_CACHE_NAME)
        {
            local_cache_ptr = new WTinylfuLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::COVERED_CACHE_NAME)
        {
            local_cache_ptr = new CoveredLocalCache(edge_wrapper_ptr, edge_idx, capacity_bytes, local_uncached_capacity_bytes, peredge_synced_victimcnt);
        }
        else
        {
            std::ostringstream oss;
            oss << "local edge cache " << cache_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(local_cache_ptr != NULL);
        return local_cache_ptr;
    }

    LocalCacheBase::LocalCacheBase(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : capacity_bytes_(capacity_bytes), edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << base_instance_name_ << " " << "rwlock_for_local_cache_ptr_";
        rwlock_for_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_local_cache_ptr_ != NULL);
    }

    LocalCacheBase::~LocalCacheBase()
    {
        assert(rwlock_for_local_cache_ptr_ != NULL);
        delete rwlock_for_local_cache_ptr_;
        rwlock_for_local_cache_ptr_ = NULL;
    }

    // (1) Check is cached and access validity

    bool LocalCacheBase::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock to check local metadata atomically
        std::string context_name = "LocalCacheBase::isLocalCached()";
        rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);

        bool is_cached = isLocalCachedInternal_(key);

        rwlock_for_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool LocalCacheBase::getLocalCache(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        checkPointers_();

        // Acquire a write lock to update local metadata atomically
        std::string context_name = "LocalCacheBase::getLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        bool is_local_cached = getLocalCacheInternal_(key, is_redirected, value, affect_victim_tracker);

        // Object size checking
        if (is_local_cached)
        {
            const bool is_valid_objsize = isValidObjsize_(key, value);
            assert(is_valid_objsize);
        }

        rwlock_for_local_cache_ptr_->unlock(context_name);

        return is_local_cached;
    }

    bool LocalCacheBase::updateLocalCache(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        checkPointers_();

        // Acquire a write lock for local metadata to update local metadata atomically (so no need to hack LFU cache)
        std::string context_name = "LocalCacheBase::updateLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        is_successful = false;
        bool is_local_cached = updateLocalCacheInternal_(key, value, is_getrsp, is_global_cached, affect_victim_tracker, is_successful);

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool LocalCacheBase::needIndependentAdmit(const Key& key, const Value& value) const
    {
        checkPointers_();

        // Object size checking
        const bool is_valid_objsize = isValidObjsize_(key, value);
        if (!is_valid_objsize)
        {
            return false;
        }

        // Acquire a write lock for local metadata to update local metadata atomically (so no need to hack LFU cache)
        std::string context_name = "LocalCacheBase::needIndependentAdmit()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        bool need_independent_admit = needIndependentAdmitInternal_(key, value);

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return need_independent_admit;
    }

    void LocalCacheBase::admitLocalCache(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        checkPointers_();

        // Acquire a write lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::admitLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        // NOTE: MUST with valid object size, as baselines always return false in needIndependentAdmitInternal_() if object size is too large, while COVERED NEVER track large objects in local uncached metadata and hence NEVER trigger normal/fast-path placement for them
        const bool is_valid_objsize = isValidObjsize_(key, value);
        assert(is_valid_objsize); // Object size checking

        is_successful = false;
        admitLocalCacheInternal_(key, value, is_neighbor_cached, affect_victim_tracker, is_successful);

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return;
    }

    bool LocalCacheBase::getLocalCacheVictimKeys(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        checkPointers_();

        assert(hasFineGrainedManagement());

        // Acquire a read lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::getLocalCacheVictimKeys()";
        rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);

        // NOTE: although we ONLY track and admit popular uncached objects w/ reasonable object sizes, required size could still exceed max valid object size due to multiple admissions in parallel -> NO need to check required size
        //assert(isValidObjsize_(static_cast<uint32_t>(required_size)));

        bool has_victim_key = getLocalCacheVictimKeysInternal_(keys, victim_cacheinfos, required_size);

        // Object size checking
        for (std::list<VictimCacheinfo>::const_iterator victim_const_iter = victim_cacheinfos.begin(); victim_const_iter != victim_cacheinfos.end(); victim_const_iter++)
        {
            ObjectSize tmp_victim_objsize = 0;
            bool is_complete = victim_const_iter->getObjectSize(tmp_victim_objsize);
            assert(is_complete);
            const bool is_valid_objsize = isValidObjsize_(tmp_victim_objsize);
            assert(is_valid_objsize);
        }

        rwlock_for_local_cache_ptr_->unlock_shared(context_name);
        return has_victim_key;
    }

    bool LocalCacheBase::evictLocalCacheWithGivenKey(const Key& key, Value& value)
    {
        checkPointers_();

        assert(hasFineGrainedManagement());

        // Acquire a write lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::evictLocalCacheIfKeyMatch()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        bool is_evict = evictLocalCacheWithGivenKeyInternal_(key, value);

        // Object size checking (NOTE: we will never admit large-objsize objects into edge cache)
        if (is_evict)
        {
            const bool is_valid_objsize = isValidObjsize_(key, value);
            assert(is_valid_objsize);
        }

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return is_evict;
    }

    void LocalCacheBase::evictLocalCacheNoGivenKey(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        checkPointers_();

        assert(!hasFineGrainedManagement());

        // Acquire a write lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::evictLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        evictLocalCacheNoGivenKeyInternal_(victims, required_size);

        // Object size checking
        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator victim_const_iter = victims.begin(); victim_const_iter != victims.end(); ++victim_const_iter)
        {
            const Key& tmp_victim_key = victim_const_iter->first;
            const Value& tmp_victim_value = victim_const_iter->second;

            const bool is_valid_objsize = isValidObjsize_(tmp_victim_key, tmp_victim_value);
            assert(is_valid_objsize);
        }

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return;
    }

    // (4) Other functions

    void LocalCacheBase::invokeCustomFunction(const std::string& func_name, CustomFuncParamBase* func_param_ptr)
    {
        checkPointers_();

        std::string context_name = "LocalCacheBase::invokeCustomFunction(" + func_name + ")";

        const bool is_local_cache_write_lock = func_param_ptr->isLocalCacheWriteLock();
        if (is_local_cache_write_lock)
        {
            // Acquire a write lock for local metadata to invoke method-specific function atomically
            rwlock_for_local_cache_ptr_->acquire_lock(context_name);
        }
        else
        {
            // Acquire a read lock for local metadata to invoke method-specific function atomically
            rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);
        }

        invokeCustomFunctionInternal_(func_name, func_param_ptr);

        if (is_local_cache_write_lock)
        {
            rwlock_for_local_cache_ptr_->unlock(context_name);
        }
        else
        {
            rwlock_for_local_cache_ptr_->unlock_shared(context_name);
        }

        return;
    }

    uint64_t LocalCacheBase::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::getSizeForCapacity()";
        rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = getSizeForCapacityInternal_();

        rwlock_for_local_cache_ptr_->unlock_shared(context_name);

        return internal_size;
    }

    void LocalCacheBase::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(rwlock_for_local_cache_ptr_ != NULL);
        checkPointersInternal_();
        return;
    }

    bool LocalCacheBase::isValidObjsize_(const ObjectSize& objsize) const
    {
        bool is_valid_objsize = false;

        // Compare objsize with capacity
        is_valid_objsize = (capacity_bytes_ > objsize);

        // Custom object size checking
        if (is_valid_objsize)
        {
            is_valid_objsize = checkObjsizeInternal_(objsize);
        }

        return is_valid_objsize;
    }

    bool LocalCacheBase::isValidObjsize_(const Key& key, const Value& value) const
    {
        const ObjectSize object_size = key.getKeyLength() + value.getValuesize();
        return isValidObjsize_(object_size);
    }
}