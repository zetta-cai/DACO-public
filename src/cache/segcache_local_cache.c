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
        // NOTE: we limit cache capacity outside SegcacheLocalCache (in EdgeWrapper); here we set segcache local cache size as overall cache capacity to avoid cache capacity constraint inside SegcacheLocalCache (too small cache capacity cannot support segment-based memory allocation in SegCache (see src/cache/segcache/benchmarks/storage_seg/storage_seg.c))
        uint64_t over_provisioned_capacity_bytes = capacity_bytes + COMMON_ENGINE_INTERNAL_UNUSED_CAPACITY_BYTES; // Just avoid internal eviction yet NOT affect cooperative edge caching
        if (over_provisioned_capacity_bytes >= SEGCACHE_ENGINE_MIN_CAPACITY_BYTES)
        {
            segcache_options_ptr_->heap_mem.val.vuint = over_provisioned_capacity_bytes;
        }
        else
        {
            segcache_options_ptr_->heap_mem.val.vuint = SEGCACHE_ENGINE_MIN_CAPACITY_BYTES;
        }

        // Prepare metrics for SegCache
        segcache_metrics_cnt_ = METRIC_CARDINALITY(seg_metrics_st);
        if (segcache_metrics_cnt_ > 0)
        {
            segcache_metrics_ptr_ = (seg_metrics_st*)malloc(sizeof(struct metric) * segcache_metrics_cnt_); // Allocate space for metrics
            assert(segcache_metrics_ptr_ != NULL);
            *segcache_metrics_ptr_ = (seg_metrics_st){SEG_METRIC(METRIC_INIT)}; // Initialize metrics
        }

        // Initialize struct SegCache
        segcache_cache_ptr_ = (struct SegCache*)malloc(sizeof(struct SegCache));
        assert(segcache_cache_ptr_ != NULL);
        initialize_segcache(segcache_cache_ptr_);

        // Steup necessary componenets
        time_setup(NULL, segcache_cache_ptr_); // Initialize segcache's proc_sec to correctly set create_at when creating each segment
        ////hotkey_setup(&setting.hotkey);
        seg_setup(segcache_options_ptr_, segcache_metrics_ptr_, segcache_cache_ptr_);
        ////admin_process_setup();

        // NOTE: uncomment the following code and set HAVE_LOGGING = ON in CmakeLists.txt to enable debug log in segcache (ONLY support a single instance now!!!)
        //debug_setup(NULL);
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
        std::string key_str = key.getKeystr();
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key_str.length());
        key_bstr.data = key_str.data();

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Will increase read refcnt of segment
        bool is_cached = (item_ptr != NULL);
        if (is_cached)
        {
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool SegcacheLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        
        std::string key_str = key.getKeystr();
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key_str.length());
        key_bstr.data = key_str.data();

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (item_ptr != NULL);
        if (is_local_cached)
        {
            //std::string value_str(item_val(item_ptr), item_nval(item_ptr));
            value = Value(item_nval(item_ptr));
            
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        return is_local_cached;
    }

    std::list<VictimCacheinfo> SegcacheLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void SegcacheLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool SegcacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        
        const bool is_insert = false;
        is_successful = false;
        bool is_local_cached = appendLocalCache_(key, value, is_insert, is_successful);

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool SegcacheLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        // SegCache cache uses default independent admission policy (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        const bool is_valid_valuesize = ((key.getKeyLength() + value.getValuesize()) <= segcache_cache_ptr_->heap_ptr->seg_size);
        return !is_local_cached && is_valid_valuesize;
    }

    void SegcacheLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
        is_successful = false;

        // NOTE: MUST with a valid value length, as we always return false in needIndependentAdmitInternal_() if object size is too large
        assert((key.getKeyLength() + value.getValuesize()) <= segcache_cache_ptr_->heap_ptr->seg_size);

        // NOTE: admission is the same as update for SegCache due to log-structured design
        const bool is_insert = true;
        bool is_local_cached = appendLocalCache_(key, value, is_insert, is_successful);
        assert(!is_local_cached);
        assert(is_successful); // Value MUST be inserted successfully, as we have checked value length in needIndependentAdmitInternal_()

        return;
    }

    bool SegcacheLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeysInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool SegcacheLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheWithGivenKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    // NOTE: even if we allocate internal unused capacity, segcache could still have NO free segment for seg_get_new() and we still cannot avoid internal eviction. Here are the reasons and solution details:
    // (i) Even if the last segment is NOT full, due to cache capacity limitation of cooperative edge cache, segcache wrapper will invoke seg_merge_evict() to release segs_to_merge[0]
    // (ii) Segcache's implementation of seg_merge_evict() assumes that the last segment is already full and segs_to_merge[0] needs to be linked to tail as a new last segment, so it will NOT add into segs_to_merge[0] free pool
    // (iii) As the existing last segment is NOT full, segcache wrapper will NOT link it to the tail as a new last segment (i.e., waste a segment which should be free), subsequent update/admit will still write the existing last segment
    // (iv) As we have released a complete segment (i.e., segs_to_merge[0]) during seg_merge_evict(), fulling the remaining/incomplete space of existing last segment will NOT achieve capacity limitation of cooperative edge cache
    // (v) We will use seg_get_new() for a new segment to write more objects until achieving capacity limitation of cooperative edge cache, and repeat from step (i)...
    // (vi) In each iteration, we will waste one segment which should be free, and finally all free segments will be used up no matter how much internal unused capacity is allocated!!!
    void SegcacheLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
#define MAX_RETRIES 8
        assert(!hasFineGrainedManagement());

        UNUSED(required_size);

        // Refer to src/cache/segcache/src/storage/seg/seg.c::seg_evict()

        evict_rstatus_e status;
        int32_t seg_id_ret = -1;
        int n_retries_left = MAX_RETRIES;

        struct bstring* key_bstrs = NULL;
        struct bstring* value_bstrs = NULL;
        uint32_t victim_cnt = 0;

        const bool is_seg_get_new = false; // NOTE: we use seg_merge_evict() for purely eviction instead of getting a new free segment for immediate use
        while (seg_id_ret == -1 && n_retries_left >= 0)
        {
            /* evict seg */
            if (segcache_cache_ptr_->evict_info_ptr->policy == EVICT_MERGE_FIFO)
            {
                status = seg_merge_evict(&seg_id_ret, segcache_cache_ptr_, true, &key_bstrs, &value_bstrs, &victim_cnt, is_seg_get_new);
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
                oss << "retry " << n_retries_left << " in evictLocalCacheInternal_()";
                Util::dumpWarnMsg(instance_name_, oss.str());

                INCR(segcache_cache_ptr_->seg_metrics, seg_evict_retry);
            }
        }

        // NOTE: we use seg_merge_evict() for purely eviction instead of getting a new free segment for immediate use -> NO need to update stats for seg get exceptions, and also NO need to initialize the new segment (this will break the prev/next seg ids set by seg_add_to_freepool())
        UNUSED(seg_id_ret);
        /*if (seg_id_ret == -1)
        {
            INCR(segcache_cache_ptr_->seg_metrics, seg_get_ex);
            Util::dumpErrorMsg(instance_name_, "unable to get new seg from eviction");
            exit(-1);
        }
        seg_init(seg_id_ret, segcache_cache_ptr_);*/

        // Return evicted key-value pairs outside SegcacheLocalCache
        if (victim_cnt > 0)
        {
            for (uint32_t i = 0; i < victim_cnt; i++)
            {
                Key tmp_key(std::string(key_bstrs[i].data, key_bstrs[i].len));
                Value tmp_value(value_bstrs[i].len);
                victims.insert(std::pair<Key, Value>(tmp_key, tmp_value));

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

    void SegcacheLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, void* func_param_ptr)
    {
        Util::dumpErrorMsg(instance_name_, "updateLocalCacheMetadataInternal_() is ONLY for COVERED
        !");
        exit(1);
        return;
    }

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

    // NOTE: appendLocalCache_() invoked by update/admit will append data into the last segment, which may trigger seg_get_new() if last seg is full
    // -> If NO free segment (except the reserved segment), seg_get_new() will invoke seg_merge_evict() to release a free segment and link to tail as last segment for update/admit
    // -> This will evict cached objects inside segcache engine (i.e., internal eviction unawared by segcache wrapper) and hence incur inconsistent dirinfo
    // Solutions:
    // (i) We have allocated internal unused capacity if with internal cache engine (e.g., segcache, cachelib, and covered) to avoid internal eviction
    // (ii) Even if we allocate internal unused capacity, segcache could still have NO free segment for seg_get_new() due to incorrect usage of seg_merge_evict() -> we have passed is_seg_get_new = false into seg_merge_evict() to fix NO free segment issue
    bool SegcacheLocalCache::appendLocalCache_(const Key& key, const Value& value, const bool& is_insert, bool& is_successful)
    {
        bool is_local_cached = true;
        is_successful = false; // Whether the value is updated/inserted successfully

        std::string keystr = key.getKeystr();
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(keystr.length());
        key_bstr.data = keystr.data();

        std::string valuestr = value.generateValuestr();
        struct bstring value_bstr;
        value_bstr.len = static_cast<uint32_t>(valuestr.length());
        value_bstr.data = valuestr.data();

        // Check whether key has already been cached
        struct item* prev_item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        is_local_cached = (prev_item_ptr != NULL);
        if (is_local_cached) // prev_item_ptr != NULL
        {
            item_release(prev_item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        // Check value length vs. segment size
        if (!is_insert) // Not write for update w/ too large object size
        {
            if ((key.getKeyLength() + value.getValuesize()) > segcache_cache_ptr_->heap_ptr->seg_size)
            {
                is_successful = false;
                return is_local_cached;
            }
        }
        else // NOTE: value length MUST be valid for insert, as we have checked value length in needIndependentAdmitInternal_()
        {
            assert((key.getKeyLength() + value.getValuesize()) <= segcache_cache_ptr_->heap_ptr->seg_size);
        }

        if (is_insert || is_local_cached) // Always write for insert, yet only write for update if key is cached
        {
            // Append current item for new value if key is cached (will trigger independent admission later if key is NOT cached now)
            struct item *cur_item_ptr = NULL;
            delta_time_i tmp_ttl = 0; // NOTE: TTL of 0 is okay, as we will always disable timeout-based expiration (out of out scope) in segcache during cooperative edge caching
            item_rstatus_e status = item_reserve_with_ttl(&cur_item_ptr, &key_bstr, &value_bstr, value_bstr.len, 0, tmp_ttl, segcache_cache_ptr_); // Append item to segment
            assert(status == ITEM_OK);
            item_insert(cur_item_ptr, segcache_cache_ptr_); // Update hashtable for newly-appended item

            is_successful = true;
        }

        return is_local_cached;
    }

}