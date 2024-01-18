#include "cache/frozenhot_local_cache.h"

#include <assert.h>
#include <functional> // std::function, std::bind, std::placeholders
#include <sched.h> // sched_getcpu
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string FrozenhotLocalCache::kClassName("FrozenhotLocalCache");

    FrozenhotLocalCache::FrozenhotLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        const size_t num_shards = 1; // NOTE: we have simulated multiple edge nodes with intra-node partitioning (i.e., sharding) outside FrozenhotLocalCache
        const int rebuild_freq = 20; // NOTE: rebuild frozen cache every (20X construct steps) cache accesses, which always has large throughput for Twitter datasets in the original paper of FrozenHot, to trade-off rebuilding overhead and dynamic cache access overhead
        //frozenhot_cache_ptr_ = new FrozenhotCache(capacity_bytes, num_shards, FrozenhotCache::FIFO_FH, rebuild_freq); // NOTE: we use FIFO-FH by default, which has the smallest average cache hit latency in the original paper of FrozenHot
        frozenhot_cache_ptr_ = new FrozenhotCache(capacity_bytes, num_shards, FrozenhotCache::LRU_FH, rebuild_freq); // NOTE: we use LRU-FH by default, which has the largest throughput in the original paper of FrozenHot
        assert(frozenhot_cache_ptr_ != NULL);

        // NOTE: refer to lib/frozenhot/test_trace.cpp and warmup() in lib/frozenhot/traceloader/client.h to prepare for FrozenHot cache
        // (1) NOTE: NOT launch worker threads to disable internal cache accessing, yet use thread_init to initialize DLHT GC for current CPU core ID if not (see threadInitIfNot_())
        thread_init_set_.clear();
        // (2) NOTE: launch fast cache monitor to construct FrozenHot cache periodically (NOTE: Not bind any specific CPU core, as we don't want to hardcode CPU core ID as in the original FrozenHot project)
        std::function<void()> fn_monitor = std::bind(&FrozenhotCache::FastHashMonitor, frozenhot_cache_ptr_);
        monitor_thread_ptr_ = new std::thread(fn_monitor);
        assert(monitor_thread_ptr_ != NULL);
    }
    
    FrozenhotLocalCache::~FrozenhotLocalCache()
    {
        // Wait for FrozenHot monitor thread
        frozenhot_cache_ptr_->monitor_stop(); // Set FrozenhotCache::should_stop as true such that the background monitor thread can break the while loop
        monitor_thread_ptr_->join();

        // Release monitor thread
        assert(monitor_thread_ptr_ != NULL);
        delete monitor_thread_ptr_;
        monitor_thread_ptr_ = NULL;

        // Release FrozenHot cache
        assert(frozenhot_cache_ptr_ != NULL);
        delete frozenhot_cache_ptr_;
        frozenhot_cache_ptr_ = NULL;
    }

    const bool FrozenhotLocalCache::hasFineGrainedManagement() const
    {
        return true; // Key-level (i.e., object-level) cache management
    }

    // (1) Check is cached and access validity

    bool FrozenhotLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        bool is_cached = frozenhot_cache_ptr_->exists(key);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool FrozenhotLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        bool is_local_cached = frozenhot_cache_ptr_->find(value, key);

        return is_local_cached;
    }

    bool FrozenhotLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // Check is local cached
        bool is_local_cached = frozenhot_cache_ptr_->exists(key);
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            bool tmp_is_local_cached = frozenhot_cache_ptr_->update(key, value);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool FrozenhotLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // LRB cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void FrozenhotLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // ONLY for COVERED

        bool is_local_cached = isLocalCachedInternal_(key);
        if (!is_local_cached) // NOTE: NOT admit if key exists
        {
            bool is_uncached = frozenhot_cache_ptr_->insert(key, value);
            assert(is_uncached == true);
            is_successful = true;
        }

        return;
    }

    bool FrozenhotLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        UNUSED(required_size); // NO need to provide multiple victims based on required size due to without victim fetching
        UNUSED(victim_cacheinfos); // ONLY for COVERED

        Key tmp_victim_key;
        bool has_victim_key = frozenhot_cache_ptr_->findVictimKey(tmp_victim_key);
        if (has_victim_key)
        {
            if (keys.find(tmp_victim_key) == keys.end())
            {
                keys.insert(tmp_victim_key);   
            }
        }

        return has_victim_key;
    }

    bool FrozenhotLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        bool is_evict = frozenhot_cache_ptr_->evict(key, value);

        return is_evict;
    }

    void FrozenhotLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheNoGivenKeyInternal_() is not supported due to fine-grained management");
        exit(1);

        return;
    }

    // (4) Other functions

    void FrozenhotLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t FrozenhotLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = frozenhot_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void FrozenhotLocalCache::threadInitIfNot_() const
    {
        int current_cpuid = sched_getcpu();
        assert(current_cpuid >= 0);
        if (thread_init_set_.find(current_cpuid) == thread_init_set_.end()) // Not initialized before
        {
            frozenhot_cache_ptr_->thread_init(current_cpuid); // Initialize CLHT GC for current CPU core ID
            thread_init_set_.insert(static_cast<uint32_t>(current_cpuid));
        }
        return;
    }

    void FrozenhotLocalCache::checkPointersInternal_() const
    {
        assert(frozenhot_cache_ptr_ != NULL);
        return;
    }

    bool FrozenhotLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }
}