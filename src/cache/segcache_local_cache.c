#include "cache/segcache_local_cache.h"

#include <assert.h>
#include <sstream>

extern "C" { // Such that compiler will use C naming convention for functions instead of C++
// NOTE: as we have set include path for cache/segcache/src/deps/ccommon, the following header files are also our hacked version
#include <cc_bstring.h> // struct bstring
#include <cc_option.h> // struct option, OPTION_CARDINALITY, OPTION_INIT, option_load_default
#include <cc_metric.h> // struct metric, METRIC_CARDINALITY, METRIC_INIT

#include "cache/segcache/src/storage/seg/item.h" // item_rstatus_e
#include "cache/segcache/src/storage/seg/segevict.h" // evict_rstatus_e
#include "cache/segcache/src/time/time.h" // delta_time_i
}

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

        // Prepare options for SegCache
        segcache_options_cnt_ = OPTION_CARDINALITY(seg_options_st);
        assert(segcache_options_cnt_ > 0);
        segcache_options_ptr_ = (seg_options_st*)malloc(sizeof(struct option) * segcache_options_cnt_); // Allocate space for options
        assert(segcache_options_ptr_ != NULL);
        CPP_SEG_OPTION(CPP_OPTION_INIT, segcache_options_ptr_); // Initialize default values in options
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

        // NOTE: uncomment to enable debug log in segcache (ONLY support a single instance now!!!)
        debug_setup(NULL); // TMPDEBUG0814
    }
    
    SegcacheLocalCache::~SegcacheLocalCache()
    {
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

    const bool SegcacheLocalCache::hasFineGrainedManagement() const
    {
        return false; // Segment-level cache management
    }

    // (1) Check is cached and access validity

    bool SegcacheLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key.getKeystr().length());
        key_bstr.data = key.getKeystr().data();

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Will increase read refcnt of segment
        bool is_cached = (item_ptr != NULL);
        if (is_cached)
        {
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        // TMPDEBUG0814
        Util::dumpVariablesForDebug(instance_name_, 4, "isLocalCachedInternal_() for key:", key.getKeystr().c_str(), "is_local_cached:", Util::toString(is_cached).c_str());

        return is_cached;
    }

    // (2) Access local edge cache

    bool SegcacheLocalCache::getLocalCacheInternal_(const Key& key, Value& value) const
    {
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key.getKeystr().length());
        key_bstr.data = key.getKeystr().data();

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (item_ptr != NULL);
        if (is_local_cached)
        {
            //std::string value_str(item_val(item_ptr), item_nval(item_ptr));
            value = Value(item_nval(item_ptr));
            
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        // TMPDEBUG0814
        Util::dumpVariablesForDebug(instance_name_, 4, "getLocalCacheInternal_() for key:", key.getKeystr().c_str(), "is_local_cached:", Util::toString(is_local_cached).c_str());

        return is_local_cached;
    }

    bool SegcacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value)
    {
        bool is_local_cached = appendLocalCache_(key, value);

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool SegcacheLocalCache::needIndependentAdmitInternal_(const Key& key) const
    {
        // SegCache cache uses default independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        return true;
    }

    void SegcacheLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value)
    {
        // NOTE: admission is the same as update for SegCache due to log-structured design
        bool is_local_cached = appendLocalCache_(key, value);
        assert(!is_local_cached);

        return;
    }

    bool SegcacheLocalCache::getLocalCacheVictimKeyInternal_(Key& key, const Key& admit_key, const Value& admit_value) const
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool SegcacheLocalCache::evictLocalCacheIfKeyMatchInternal_(const Key& key, Value& value, const Key& admit_key, const Value& admit_value)
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheIfKeyMatchInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    void SegcacheLocalCache::evictLocalCacheInternal_(std::vector<Key>& keys, std::vector<Value>& values, const Key& admit_key, const Value& admit_value)
    {
#define MAX_RETRIES 8
        assert(!hasFineGrainedManagement());

        // TMPDEBUG0814
        Util::dumpVariablesForDebug(instance_name_, 2, "evictLocalCacheInternal_() for admit_key:", admit_key.getKeystr().c_str());

        UNUSED(admit_key);
        UNUSED(admit_value);

        // Refer to src/cache/segcache/src/storage/seg/seg.c::seg_evict()

        evict_rstatus_e status;
        int32_t seg_id_ret = -1;
        int n_retries_left = MAX_RETRIES;

        struct bstring* key_bstrs = NULL;
        struct bstring* value_bstrs = NULL;
        uint32_t victim_cnt = 0;

        while (seg_id_ret == -1 && n_retries_left >= 0)
        {
            /* evict seg */
            if (segcache_cache_ptr_->evict_info_ptr->policy == EVICT_MERGE_FIFO)
            {
                status = seg_merge_evict(&seg_id_ret, segcache_cache_ptr_, true, &key_bstrs, &value_bstrs, &victim_cnt);
            }
            else
            {
                status = seg_evict(&seg_id_ret, segcache_cache_ptr_, true, &key_bstrs, &value_bstrs, &victim_cnt);
            }

            if (status == EVICT_OK)
            {
                break;
            }

            // If NOT return EVICT_OK, MUST NO key-value pair has been evicted
            assert(key_bstrs == NULL);
            assert(value_bstrs == NULL);
            assert(victim_cnt == 0);

            if (--n_retries_left < MAX_RETRIES)
            {
                std::ostringstream oss;
                oss << "retry " << n_retries_left << "in evictLocalCacheIfKeyMatchInternal_()";
                Util::dumpWarnMsg(instance_name_, oss.str());

                INCR(segcache_cache_ptr_->seg_metrics, seg_evict_retry);
            }
        }

        if (seg_id_ret == -1)
        {
            INCR(segcache_cache_ptr_->seg_metrics, seg_get_ex);
            Util::dumpErrorMsg(instance_name_, "unable to get new seg from eviction");
            exit(-1);
        }

        seg_init(seg_id_ret, segcache_cache_ptr_);

        // Return evicted key-value pairs outside SegcacheLocalCache
        if (victim_cnt > 0)
        {
            for (uint32_t i = 0; i < victim_cnt; i++)
            {
                Key tmp_key(std::string(key_bstrs[i].data, key_bstrs[i].len));
                keys.push_back(tmp_key);
                Value tmp_value(value_bstrs[i].len);
                values.push_back(tmp_value);

                // Release data of key bstring
                free(key_bstrs[i].data);
                key_bstrs[i].len = 0;
                key_bstrs[i].data = NULL;

                // Release data of value bstring
                free(value_bstrs[i].data);
                value_bstrs[i].len = 0;
                value_bstrs[i].data = NULL;
            }

            // Release bstring structs
            free(key_bstrs);
            key_bstrs = NULL;
            free(value_bstrs);
            value_bstrs = NULL;
            victim_cnt = 0;
        }

        return;
    }

    // (4) Other functions

    uint64_t SegcacheLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = get_segcache_size_bytes(segcache_cache_ptr_);

        return internal_size;
    }

    void SegcacheLocalCache::checkPointersInternal_() const
    {
        assert(segcache_options_ptr_ != NULL);
        if (segcache_metrics_cnt_ > 0)
        {
            assert(segcache_metrics_ptr_ != NULL);
        }
        assert(segcache_cache_ptr_ != NULL);
        return;
    }

    // (5) SegCache-specific functions

    bool SegcacheLocalCache::appendLocalCache_(const Key& key, const Value& value)
    {
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key.getKeystr().length());
        key_bstr.data = key.getKeystr().data();

        std::string valuestr = value.generateValuestr();
        struct bstring value_bstr;
        value_bstr.len = static_cast<uint32_t>(valuestr.length());
        value_bstr.data = valuestr.data();

        // Check whether key has already been cached
        struct item* prev_item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (prev_item_ptr != NULL);
        if (is_local_cached)
        {
            item_release(prev_item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        // TMPDEBUG0814
        Util::dumpVariablesForDebug(instance_name_, 4, "appendLocalCache_() for key:", key.getKeystr().c_str(), "is_local_cached:", Util::toString(is_local_cached).c_str());

        // Append current item for new value no matter key has been cached or not
        struct item *cur_item_ptr = NULL;
        delta_time_i tmp_ttl = 0; // NOTE: TTL of 0 is okay, as we will always disable timeout-based expiration (out of out scope) in segcache during cooperative edge caching
        item_rstatus_e status = item_reserve_with_ttl(&cur_item_ptr, &key_bstr, &value_bstr, value_bstr.len, 0, tmp_ttl, segcache_cache_ptr_); // Append item to segment
        assert(status == ITEM_OK);
        item_insert(cur_item_ptr, segcache_cache_ptr_); // Update hashtable for newly-appended item

        // TMPDEBUG0814
        Value test_value;
        bool test_is_local_cached = getLocalCacheInternal_(key, test_value);
        Util::dumpVariablesForDebug(instance_name_, 4, "test_is_local_cached:", Util::toString(test_is_local_cached).c_str(), "test_value size:", std::to_string(test_value.getValuesize()).c_str());

        return is_local_cached;
    }

}