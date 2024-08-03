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

    #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
    SegcacheLocalCache::SegcacheLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes), logical_seg_size_(MB2B(1)), physical_seg_size_(MB2B(1))
    #else
    SegcacheLocalCache::SegcacheLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    #endif
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

        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // NOTE: cannot change 1MiB segment size, as segcache hardcodes to use 20 bits for segment offset
        // // NOTE: use 512B as physical segment size such that one segment can store ~50 objects (tens of bytes keys and 4B values) for impl trick of only storing value size instead of value content
        // segcache_options_ptr_->seg_size.val.vuint = physical_seg_size_;
        #endif

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

        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // Intialize extra bytes for impl trick of not storing value content
        extra_size_bytes_ = 0;
        #endif
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
        UNUSED(is_redirected); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        
        std::string key_str = key.getKeystr();
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(key_str.length());
        key_bstr.data = key_str.data();

        struct item* item_ptr = item_get(&key_bstr, NULL, segcache_cache_ptr_); // Lookup hashtable to get item in segment; will increase read refcnt of segment
        bool is_local_cached = (item_ptr != NULL);
        if (is_local_cached)
        {
            #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
            // NOTE: store value size instead of value content to avoid memory usage bug of segcache, yet not affect cache stable performance, as we use extra_size_bytes_ to complete the remaining bytes which should stored in segcache
            assert(item_nval(item_ptr) == sizeof(uint32_t));
            uint32_t tmp_value_size = 0;
            memcpy((char*)&tmp_value_size, item_val(item_ptr), sizeof(uint32_t));
            value = Value(tmp_value_size);
            #else
            //std::string value_str(item_val(item_ptr), item_nval(item_ptr));
            value = Value(item_nval(item_ptr));
            #endif
            
            item_release(item_ptr, segcache_cache_ptr_); // Decrease read refcnt of segment
        }

        return is_local_cached;
    }

    bool SegcacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_getrsp); // ONLY used by COVERED
        UNUSED(is_global_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        
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
        return !is_local_cached;
    }

    void SegcacheLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful, const uint64_t& miss_latency_us)
    {
        UNUSED(is_neighbor_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        UNUSED(miss_latency_us); // ONLY used by LA-Cache
        
        is_successful = false;

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
                #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
                // NOTE: store value size instead of value content to avoid memory usage bug of segcache, yet not affect cache stable performance, as we use extra_size_bytes_ to complete the remaining bytes which should stored in segcache
                assert(value_bstrs[i].len == sizeof(uint32_t));
                uint32_t tmp_value_size = 0;
                memcpy((char*)&tmp_value_size, value_bstrs[i].data, sizeof(uint32_t));
                Value tmp_value(tmp_value_size);
                if (tmp_value_size > sizeof(uint32_t)) // Update remaining bytes if necessary
                {
                    uint32_t tmp_complete_value_size = tmp_value_size - sizeof(uint32_t);
                    if (extra_size_bytes_ >= tmp_complete_value_size)
                    {
                        extra_size_bytes_ -= tmp_complete_value_size;
                    }
                    else
                    {
                        extra_size_bytes_ = 0;
                    }
                }
                #else
                Value tmp_value(value_bstrs[i].len);
                #endif

                Key tmp_key(std::string(key_bstrs[i].data, key_bstrs[i].len));
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

    void SegcacheLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void SegcacheLocalCache::invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "invokeConstCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t SegcacheLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = get_segcache_size_bytes(segcache_cache_ptr_);

        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // Consider remaining bytes to complete for the impl trick of only storing value size instead of value content
        internal_size += extra_size_bytes_;
        #endif

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
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        bool is_local_cached = true;
        is_successful = false; // Whether the value is updated/inserted successfully

        std::string keystr = key.getKeystr();
        struct bstring key_bstr;
        key_bstr.len = static_cast<uint32_t>(keystr.length());
        key_bstr.data = keystr.data(); // Shallow copy, while segcache will perform deep copy

        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // NOTE: store value size instead of value content to avoid memory usage bug of segcache, yet not affect cache stable performance, as we use extra_size_bytes_ to complete the remaining bytes which should stored in segcache
        uint32_t tmp_value_size = value.getValuesize();
        std::string tmp_value_size_str((const char*)&tmp_value_size, sizeof(uint32_t));
        struct bstring value_bstr;
        value_bstr.len = sizeof(uint32_t);
        value_bstr.data = tmp_value_size_str.data(); // Shallow copy, while segcache will perform deep copy
        #else
        std::string valuestr = value.generateValuestrForStorage();
        struct bstring value_bstr;
        value_bstr.len = static_cast<uint32_t>(valuestr.length());
        value_bstr.data = valuestr.data(); // Shallow copy, while segcache will perform deep copy
        #endif

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
            if (is_local_cached && !is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }
        }
        else // NOTE: value length MUST be valid for insert, as we have checked value length in needIndependentAdmitInternal_()
        {
            assert(is_valid_objsize);
        }

        // Insert/update with the latest value if necessary
        if (is_insert || is_local_cached) // Always write for insert, yet only write for update if key is cached
        {
            // Append current item for new value if key is cached (will trigger independent admission later if key is NOT cached now)
            struct item *cur_item_ptr = NULL;
            delta_time_i tmp_ttl = 0; // NOTE: TTL of 0 is okay, as we will always disable timeout-based expiration (out of out scope) in segcache during cooperative edge caching
            item_rstatus_e status = item_reserve_with_ttl(&cur_item_ptr, &key_bstr, &value_bstr, value_bstr.len, 0, tmp_ttl, segcache_cache_ptr_); // Append item to segment
            assert(status == ITEM_OK);
            item_insert(cur_item_ptr, segcache_cache_ptr_); // Update hashtable for newly-appended item

            #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
            // NOTE: NO need to consider original value size, as segcache uses log-structured memory and just appends new value to the end of a segment no matter insert/update; the original value will be removed (and thus the correpsonding remaining bytes will be released) during eviction later
            if (tmp_value_size > sizeof(uint32_t)) // Update remaining bytes if necessary
            {
                uint32_t tmp_complete_value_size = tmp_value_size - sizeof(uint32_t);
                extra_size_bytes_ += tmp_complete_value_size;
            }
            #endif

            is_successful = true;
        }

        return is_local_cached;
    }

    bool SegcacheLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        #ifdef ENABLE_ONLY_VALSIZE_FOR_SEGCACHE
        // NOTE: cannot change 1MiB segment size, as segcache hardcodes to use 20 bits for segment offset
        bool is_valid_objsize = objsize <= segcache_cache_ptr_->heap_ptr->seg_size;
        // // NOTE: use 1 MiB logical value size (the same as default setting of segcache) for object size checking -> will only store 4B value size into 512B logical segments to avoid memory usage bugs of segcache itself
        // bool is_valid_objsize = objsize <= logical_seg_size_;
        #else
        bool is_valid_objsize = objsize <= segcache_cache_ptr_->heap_ptr->seg_size;
        #endif

        return is_valid_objsize;
    }

}