#include "cache/segcache_local_cache.h"

#include <assert.h>
#include <sstream>

#include "src/cache/segcache/deps/ccommon/include/cc_bstring.h" // struct bstring
#include "src/cache/segcache/deps/ccommon/include/cc_option.h" // struct option, OPTION_CARDINALITY, OPTION_INIT, option_load_default
#include "src/cache/segcache/deps/ccommon/include/cc_metric.h" // struct metric, METRIC_CARDINALITY, METRIC_INIT
#include "src/cache/segcache/src/storage/seg/item.h" // item_rstatus_e
#include "src/cache/segcache/src/time/time.h" // delta_time_i
#include "common/util.h"

namespace covered
{
    const uint64_t SegcacheLocalCache::SEGCACHE_MIN_CAPACITY_BYTES = GB2B(1); // 1 GiB

    const std::string SegcacheLocalCache::kClassName("SegcacheLocalCache");

    SegcacheLocalCache::SegcacheLocalCache(const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_segcache_local_cache_ptr_";
        rwlock_for_segcache_local_cache_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);

        // Prepare options for SegCache
        segcache_options_cnt_ = OPTION_CARDINALITY(seg_options_st);
        assert(segcache_options_cnt_ > 0);
        segcache_options_ptr_ = (seg_options_st*)malloc(sizeof(struct option) * segcache_options_cnt_); // Allocate space for options
        assert(segcache_options_ptr_ != NULL);
        *segcache_options_ptr_ = (seg_options_st){SEG_OPTION(OPTION_INIT)}; // Initialize default values in options
        option_load_default((struct option*)segcache_options_ptr_, segcache_options_cnt_); // Copy default values to values in options
        // NOTE: we limit cache capacity outside SegcacheLocalCache (in EdgeWrapper); here we set segcache local cache size as overall cache capacity to avoid cache capacity constraint inside SegcacheLocalCache
        if (capacity_bytes >= SEGCACHE_MIN_CAPACITY_BYTES)
        {
            segcache_options_ptr_->heap_mem.val.vuint = capacity_bytes;
        }
        else
        {
            segcache_options_ptr_->heap_mem.val.vuint = SEGCACHE_MIN_CAPACITY_BYTES;
        }

        // Prepare metrics for SegCache
        segcache_metrics_cnt_ = METRIC_CARDINALITY(seg_metrics_st);
        if (segcache_metrics_cnt_ > 0)
        {
            segcache_metrics_ptr_ = (seg_metrics_st*)malloc(sizeof(struct metric) * segcache_metrics_cnt_); // Allocate space for metrics
            assert(segcache_metrics_ptr_ != NULL);
            *segcache_metrics_ptr_ = (seg_metrics_st){SEG_METRIC(METRIC_INIT)}; // Initialize metrics
        }

        // Initialize and setup SegCache
        segcache_cache_ptr_ = (struct SegCache*)malloc(sizeof(struct SegCache));
        assert(segcache_cache_ptr_ != NULL);
        initialize_segcache(segcache_cache_ptr_);
        seg_setup(segcache_options_ptr_, segcache_metrics_ptr_, segcache_cache_ptr_);
    }
    
    SegcacheLocalCache::~SegcacheLocalCache()
    {
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);
        delete rwlock_for_segcache_local_cache_ptr_;
        rwlock_for_segcache_local_cache_ptr_ = NULL;

        assert(segcache_options_ptr_ != NULL);
        free(segcache_options_ptr_);
        segcache_options_ptr_ = NULL;

        if (segcache_metrics_cnt_ > 0)
        {
            assert(segcache_metrics_ptr_ != NULL);
            free(segcache_metrics_ptr_);
            segcache_metrics_ptr_ = NULL;
        }

        assert(segcache_cache_ptr_ != NULL);
        seg_teardown(segcache_cache_ptr_);
        release_segcache(segcache_cache_ptr_);
        free(segcache_cache_ptr_);
        segcache_cache_ptr_ = NULL;
    }

    // (1) Check is cached and access validity

    bool SegcacheLocalCache::isLocalCached(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::isLocalCached()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        struct bstring key_bstr = {.len=key.getKeystr().length(), .data=key.getKeystr().data()};

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Will increase read refcnt of segment
        bool is_cached = (item_ptr != NULL);
        if (is_cached)
        {
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return is_cached;
    }

    // (2) Access local edge cache

    bool SegcacheLocalCache::getLocalCache(const Key& key, Value& value) const
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        struct bstring key_bstr = {.len=key.getKeystr().length(), .data=key.getKeystr().data()};

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (item_ptr != NULL);
        if (is_local_cached)
        {
            //std::string value_str(item_val(item_ptr), item_nval(item_ptr));
            value = Value(item_nval(item_ptr));
            
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    bool SegcacheLocalCache::updateLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::updateLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        struct bstring key_bstr = {.len = key.getKeystr().length(), .data = key.getKeystr().data()};
        std::string valuestr = value.generateValuestr();
        struct bstring value_bstr = {.len = valuestr.length(), .data = valuestr.data()};

        // Check whether key has already been cached
        struct item* prev_item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (prev_item_ptr != NULL);
        if (is_local_cached)
        {
            item_release(prev_item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        // Append current item for new value no matter key has been cached or not
        struct item *cur_item_ptr = NULL;
        delta_time_i tmp_ttl = 0; // NOTE: TTL of 0 is okay, as we will always disable timeout-based expiration (out of out scope) in segcache during cooperative edge caching
        item_rstatus_e status = item_reserve_with_ttl(&cur_item_ptr, &key_bstr, &value_bstr, value_bstr.len, 0, tmp_ttl, segcache_cache_ptr_); // Append item to segment
        assert(status == ITEM_OK);
        item_insert(cur_item_ptr, segcache_cache_ptr_); // Update hashtable for newly-appended item

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_local_cached;
    }

    // (3) Local edge cache management

    bool SegcacheLocalCache::needIndependentAdmit(const Key& key) const
    {
        // No need to acquire a read lock for local statistics due to returning a const boolean

        // LRU cache uses LRU-based independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void SegcacheLocalCache::admitLocalCache(const Key& key, const Value& value)
    {
        checkPointers_();

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::admitLocalCache()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        // TODO: END HERE

        lru_cache_ptr_->admit(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return;
    }

    bool SegcacheLocalCache::getLocalCacheVictimKey(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getLocalCacheVictimKey()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        bool has_victim_key = lru_cache_ptr_->getVictimKey(key);

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return has_victim_key;
    }

    bool SegcacheLocalCache::evictLocalCacheIfKeyMatch(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        checkPointers_();

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Acquire a write lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::evictLocalCacheIfKeyMatch()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock(context_name);

        bool is_evict = lru_cache_ptr_->evictIfKeyMatch(key, value);

        rwlock_for_segcache_local_cache_ptr_->unlock(context_name);
        return is_evict;
    }

    // (4) Other functions

    uint64_t SegcacheLocalCache::getSizeForCapacity() const
    {
        checkPointers_();

        // Acquire a read lock for local statistics to update local statistics atomically (so no need to hack LRU cache)
        std::string context_name = "SegcacheLocalCache::getSizeForCapacity()";
        rwlock_for_segcache_local_cache_ptr_->acquire_lock_shared(context_name);

        uint64_t internal_size = lru_cache_ptr_->getSizeForCapacity();

        rwlock_for_segcache_local_cache_ptr_->unlock_shared(context_name);
        return internal_size;
    }

    void SegcacheLocalCache::checkPointers_() const
    {
        assert(rwlock_for_segcache_local_cache_ptr_ != NULL);
        assert(segcache_options_ptr_ != NULL);
        if (segcache_metrics_cnt_ > 0)
        {
            assert(segcache_metrics_ptr_ != NULL);
        }
        assert(segcache_cache_ptr_ != NULL);
    }

}