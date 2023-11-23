#include "cache/local_cache_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "cache/cachelib_local_cache.h"
#include "cache/covered_local_cache.h"
#include "cache/lfu_local_cache.h"
#include "cache/lru_local_cache.h"
#include "cache/greedy_dual_local_cache.h"
#include "cache/segcache_local_cache.h"

namespace covered
{
    const std::string LocalCacheBase::kClassName("LocalCacheBase");

    LocalCacheBase* LocalCacheBase::getLocalCacheByCacheName(const std::string& cache_name, const uint32_t& edge_idx, const uint64_t& capacity_bytes, const uint64_t& local_uncached_capacity_bytes, const uint32_t& peredge_synced_victimcnt)
    {
        LocalCacheBase* local_cache_ptr = NULL;
        if (cache_name == Util::CACHELIB_CACHE_NAME)
        {
            local_cache_ptr = new CachelibLocalCache(edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::LRUK_CACHE_NAME || cache_name == Util::GDSIZE_CACHE_NAME || cache_name == Util::GDSF_CACHE_NAME || cache_name == Util::LFUDA_CACHE_NAME)
        {
            local_cache_ptr = new GreedyDualLocalCache(cache_name, edge_idx, capacity_bytes);
        }
        else if (cache_name == Util::COVERED_CACHE_NAME)
        {
            local_cache_ptr = new CoveredLocalCache(edge_idx, capacity_bytes, local_uncached_capacity_bytes, peredge_synced_victimcnt);
        }
        else if (cache_name == Util::LFU_CACHE_NAME)
        {
            local_cache_ptr = new LfuLocalCache(edge_idx);
        }
        else if (cache_name == Util::LRU_CACHE_NAME)
        {
            local_cache_ptr = new LruLocalCache(edge_idx);
        }
        else if (cache_name == Util::SEGCACHE_CACHE_NAME)
        {
            local_cache_ptr = new SegcacheLocalCache(edge_idx, capacity_bytes);
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

    LocalCacheBase::LocalCacheBase(const uint32_t& edge_idx)
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

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    std::list<VictimCacheinfo> LocalCacheBase::getLocalSyncedVictimCacheinfosFromLocalCache() const
    {
        checkPointers_();

        // Acquire a read lock to check local metadata atomically
        std::string context_name = "LocalCacheBase::getLocalSyncedVictimCacheinfosFromLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);

        std::list<VictimCacheinfo> local_synced_victim_cacheinfos = getLocalSyncedVictimCacheinfosFromLocalCacheInternal_();

        rwlock_for_local_cache_ptr_->unlock_shared(context_name);
        return local_synced_victim_cacheinfos;
    }

    void LocalCacheBase::getCollectedPopularityFromLocalCache(const Key& key, CollectedPopularity& collected_popularity) const
    {
        checkPointers_();

        // Acquire a read lock to get local metadata atomically
        std::string context_name = "LocalCacheBase::getCollectedPopularityFromLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock_shared(context_name);

        getCollectedPopularityFromLocalCacheInternal_(key, collected_popularity);

        rwlock_for_local_cache_ptr_->unlock_shared(context_name);
        return;
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

        // Acquire a write lock for local metadata to update local metadata atomically (so no need to hack LFU cache)
        std::string context_name = "LocalCacheBase::needIndependentAdmit()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        bool need_independent_admit = needIndependentAdmitInternal_(key, value);

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return need_independent_admit;
    }

    void LocalCacheBase::admitLocalCache(const Key& key, const Value& value, bool& affect_victim_tracker, bool& is_successful)
    {
        checkPointers_();

        // Acquire a write lock for local metadata to update local metadata atomically
        std::string context_name = "LocalCacheBase::admitLocalCache()";
        rwlock_for_local_cache_ptr_->acquire_lock(context_name);

        is_successful = false;
        admitLocalCacheInternal_(key, value, affect_victim_tracker, is_successful);

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

        bool has_victim_key = getLocalCacheVictimKeysInternal_(keys, victim_cacheinfos, required_size);

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

        rwlock_for_local_cache_ptr_->unlock(context_name);
        return;
    }

    // (4) Other functions

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
        assert(rwlock_for_local_cache_ptr_ != NULL);
        checkPointersInternal_();
        return;
    }
}