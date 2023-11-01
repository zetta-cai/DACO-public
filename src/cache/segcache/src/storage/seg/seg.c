#include "seg.h"
#include "background.h"
#include "constant.h"
#include "hashtable.h"
#include "item.h"
#include "segevict.h"
#include "ttlbucket.h"
#include "datapool/datapool.h"

#include <cc_mm.h>
#include <cc_util.h>

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <stdio.h>

#ifdef USE_PMEM
#include "libpmem.h"
#endif

#define SEG_MODULE_NAME "storage::seg"

extern struct setting        setting; // Siyuan: setting is NOT used here (ONLY used by main.c in segcache_ptr client/server)
//extern struct seg_evict_info evict_info;
extern const char            *eviction_policy_names[]; // Siyuan: NO need to encapsulate const global variables

// Siyuan: encapsulate global variables
// struct seg_heapinfo heap; /* info of all allocated segs */
// struct ttl_bucket   ttl_buckets[MAX_N_TTL_BUCKET];

// Siyuan: encapsulate global variables
// static bool           seg_initialized = false;
// seg_metrics_st        *seg_metrics    = NULL;
// seg_options_st        *seg_options    = NULL;
// seg_perttl_metrics_st perttl[MAX_N_TTL_BUCKET];

// Siyuan: encapsulate global variables
// proc_time_i   flush_at = -1;
// bool use_cas = false;
// pthread_t     bg_tid;
// int           n_thread = 1;
// volatile bool stop     = false;

// Siyuan: NO need to encapsulate const global variables
static const char *seg_state_change_str[] = {
    "allocation",
    "concurrent_get",
    "eviction",
    "force_eviction",
    "expiration",
    "invalid_reason",
};

void
dump_seg_info(struct SegCache* segcache_ptr)
{
    struct seg *seg;
    int32_t    seg_id;

    for (int32_t i = 0; i < MAX_N_TTL_BUCKET; i++) {
        seg_id = segcache_ptr->ttl_buckets[i].first_seg_id;
        if (seg_id != -1) {
            printf("ttl bucket %4d: ", i);
        }
        else {
            continue;
        }
        while (seg_id != -1) {
            seg = &segcache_ptr->heap_ptr->segs[seg_id];
            printf("seg %d (%d), ", seg_id, seg_evictable(seg, segcache_ptr));
            seg_id = seg->next_seg_id;
        }
        printf("\n");
    }

    char         s[64];
    for (int32_t j = 0; j < segcache_ptr->heap_ptr->max_nseg; j++) {
        snprintf(s, 64, "seg %4d evictable %d", j,
            seg_evictable(&segcache_ptr->heap_ptr->segs[j], segcache_ptr));
        SEG_PRINT(j, s, log_warn, segcache_ptr);
    }
}

/**
 * wait until no other threads are accessing the seg (refcount == 0)
 */
void
seg_wait_refcnt(int32_t seg_id, struct SegCache* segcache_ptr)
{
    struct seg *seg          = &segcache_ptr->heap_ptr->segs[seg_id];
    ASSERT(seg->accessible != 1);
    bool       r_log_printed = false, w_log_printed = false;
    int        r_ref, w_ref;

    w_ref = __atomic_load_n(&(seg->w_refcount), __ATOMIC_RELAXED);
    r_ref = __atomic_load_n(&(seg->r_refcount), __ATOMIC_RELAXED);

    if (w_ref) {
        log_verb("wait for seg %d refcount, current read refcount "
                 "%d, write refcount %d",
            seg_id, r_ref, w_ref);
        w_log_printed = true;
    }

    while (w_ref) {
        sched_yield();
        w_ref = __atomic_load_n(&(seg->w_refcount), __ATOMIC_RELAXED);
    }

    if (r_ref) {
        log_verb("wait for seg %d refcount, current read refcount "
                 "%d, write refcount %d",
            seg_id, r_ref, w_ref);
        r_log_printed = true;
    }

    while (r_ref) {
        sched_yield();
        r_ref = __atomic_load_n(&(seg->r_refcount), __ATOMIC_RELAXED);
    }

    if (r_log_printed || w_log_printed)
        log_verb("wait for seg %d refcount finishes", seg_id);
}

/**
 * check whether seg is accessible
 */
bool
seg_is_accessible(int32_t seg_id, struct SegCache* segcache_ptr)
{
    struct seg *seg = &segcache_ptr->heap_ptr->segs[seg_id];
    if (__atomic_load_n(&seg->accessible, __ATOMIC_RELAXED) == 0) {
        return false;
    }

    return seg->ttl + seg->create_at > time_proc_sec(segcache_ptr)
        && seg->create_at > segcache_ptr->flush_at;
}

bool
seg_w_ref(int32_t seg_id, struct SegCache* segcache_ptr)
{
    struct seg *seg = &segcache_ptr->heap_ptr->segs[seg_id];

    if (seg_is_accessible(seg_id, segcache_ptr)) {
        __atomic_fetch_add(&seg->w_refcount, 1, __ATOMIC_RELAXED);
        return true;
    }

    return false;
}

void
seg_w_deref(int32_t seg_id, struct SegCache* segcache_ptr)
{
    struct seg *seg = &segcache_ptr->heap_ptr->segs[seg_id];

    int16_t ref = __atomic_sub_fetch(&seg->w_refcount, 1, __ATOMIC_RELAXED);

    ASSERT(ref >= 0);
}

/**
 * initialize the seg and seg header
 *
 * we do not use lock in this function, because the seg being initialized either
 * comes from un-allocated heap, free pool or eviction
 * in any case - the seg is only owned by current thread,
 * the one exception is that other threads performing evictions may
 * read the seg header,
 * in order to avoid eviction algorithm picking this seg,
 * we do not clear seg->locked until it is linked into ttl_bucket
 */
void
seg_init(int32_t seg_id, struct SegCache* segcache_ptr)
{
    ASSERT(seg_id != -1);
    struct seg *seg = &segcache_ptr->heap_ptr->segs[seg_id];

#if defined DEBUG_MODE
    seg->seg_id_non_decr += segcache_ptr->heap_ptr->max_nseg;
    if (seg->seg_id_non_decr > 1ul << 23ul) {
        seg->seg_id_non_decr = seg->seg_id % segcache_ptr->heap_ptr->max_nseg;
    }
    seg->n_rm_item = 0;
    seg->n_rm_bytes = 0;
#endif

    uint8_t *data_start = get_seg_data_start(seg_id, segcache_ptr);

    ASSERT(seg->accessible == 0);
    ASSERT(seg->evictable == 0);

//    cc_memset(data_start, 0, segcache_ptr->heap_ptr->seg_size);

#if defined CC_ASSERT_PANIC || defined CC_ASSERT_LOG
    *(uint64_t *) (data_start) = SEG_MAGIC;
    seg->write_offset = 8;
    seg->live_bytes   = 8;
    seg->total_bytes  = 8;
#else
    seg->write_offset   = 0;
    seg->live_bytes  = 0;
#endif

    seg->prev_seg_id = -1;
    seg->next_seg_id = -1;

    seg->n_live_item = 0;
    seg->n_total_item = 0;

    seg->create_at = time_proc_sec(segcache_ptr);
    seg->merge_at  = 0;

    seg->accessible = 1;

    seg->n_hit         = 0;
    seg->n_active      = 0;
    seg->n_active_byte = 0;
}

void
rm_seg_from_ttl_bucket(int32_t seg_id, struct SegCache* segcache_ptr)
{
    struct seg        *seg        = &segcache_ptr->heap_ptr->segs[seg_id];
    struct ttl_bucket *ttl_bucket = &segcache_ptr->ttl_buckets[find_ttl_bucket_idx(seg->ttl)];
    ASSERT(seg->ttl == ttl_bucket->ttl);

    /* all modification to seg chain needs to be protected by lock
     * TODO(juncheng): can change to the TTL lock? */
    ASSERT(pthread_mutex_trylock(&segcache_ptr->heap_ptr->mtx) != 0);

    int32_t prev_seg_id = seg->prev_seg_id;
    int32_t next_seg_id = seg->next_seg_id;

    if (prev_seg_id == -1) {
        ASSERT(ttl_bucket->first_seg_id == seg_id);

        ttl_bucket->first_seg_id = next_seg_id;
    }
    else {
        segcache_ptr->heap_ptr->segs[prev_seg_id].next_seg_id = next_seg_id;
    }

    if (next_seg_id == -1) {
        ASSERT(ttl_bucket->last_seg_id == seg_id);

        ttl_bucket->last_seg_id = prev_seg_id;
    }
    else {
        segcache_ptr->heap_ptr->segs[next_seg_id].prev_seg_id = prev_seg_id;
    }

    ttl_bucket->n_seg -= 1;
    ASSERT(ttl_bucket->n_seg >= 0);

    log_verb("remove seg %d from ttl bucket, after removal, first seg %d,"
             "last %d, prev %d, next %d", seg_id,
        ttl_bucket->first_seg_id, ttl_bucket->last_seg_id,
        seg->prev_seg_id, seg->next_seg_id);
}

/**
 * remove all items on this segment,
 * most of the time (common case), the seg should have no writers because
 * the eviction algorithms will avoid the segment with w_refcnt > 0 and
 * segment with next_seg_id == -1 (active segment)
 *
 * However, it is possible we are evicting a segment that is
 * actively being written to, when the following happens:
 * 1. it takes too long (longer than its TTL) for the segment to
 *      finish writing and it has expired
 * 2. cache size is too small and the workload uses too many ttl buckets
 *
 *
 * because multiple threads could try to evict/expire the seg at the same time,
 * return true if current thread is able to grab the lock, otherwise false
 */
/* TODO(jason): separate into two func: one lock for remove, one remove */
bool
rm_all_item_on_seg(int32_t seg_id, enum seg_state_change reason, struct SegCache* segcache_ptr, bool need_victims, struct bstring** key_bstrs_ptr, struct bstring** value_bstrs_ptr, uint32_t* victim_cnt_ptr)
{
    ASSERT(seg_id >= 0);

    struct seg  *seg = &segcache_ptr->heap_ptr->segs[seg_id];
    struct item *it;

    /* prevent being picked by eviction algorithm concurrently */
    if (__atomic_exchange_n(&seg->evictable, 0, __ATOMIC_RELAXED) == 0) {
        /* this seg is either expiring or being evicted by other threads */

        if (reason == SEG_EXPIRATION) {
            SEG_PRINT(seg_id, "expiring unevictable seg", log_warn, segcache_ptr);

            INCR(segcache_ptr->seg_metrics, seg_evict_ex);
        }
        return false;
    }

    /* prevent future read and write access */
    __atomic_store_n(&seg->accessible, 0, __ATOMIC_RELAXED);

    /* next_seg_id == -1 indicates this is the last segment of a ttl_bucket
     * or freepool, and we should not evict the seg
     * we have tried to avoid picking such seg at eviction, but it can still
     * happen because
     * 1. this seg has been evicted and reused by another thread since it was
     *      picked by eviction algorithm (because there is no lock) - very rare
     * 2. this seg is expiring, so another thread is removing it
     * either case should be rare, it is the effect of
     * optimistic concurrency control - no lock and roll back if needed
     *
     * since we have already "locked" the seg, it will not be held by other
     * threads, so we can check again safely
     */
    if (seg->next_seg_id == -1 &&
        reason != SEG_EXPIRATION && reason != SEG_FORCE_EVICTION) {
        /* "this should not happen" */
        ASSERT(0);
//        __atomic_store_n(&seg->evictable, 0, __ATOMIC_SEQ_CST);
//        INCR(segcache_ptr->seg_metrics, seg_evict_ex);

        return false;
    }

    uint8_t  *seg_data = get_seg_data_start(seg_id, segcache_ptr);
    uint8_t  *curr     = seg_data;
    uint32_t offset    = MIN(seg->write_offset, segcache_ptr->heap_ptr->seg_size) - ITEM_HDR_SIZE;

    SEG_PRINT(seg_id, seg_state_change_str[reason], log_debug, segcache_ptr);

#if defined CC_ASSERT_PANIC || defined CC_ASSERT_LOG
    ASSERT(*(uint64_t *) (curr) == SEG_MAGIC);
    curr += sizeof(uint64_t);
#endif

    /* remove segment from TTL bucket */
    pthread_mutex_lock(&segcache_ptr->heap_ptr->mtx);
    rm_seg_from_ttl_bucket(seg_id, segcache_ptr);
    pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);

    // Siyuan: allocate space for evicted key-value pairs
    uint32_t victim_idx = 0;
    uint32_t victim_cnt = seg->n_live_item;
    if (need_victims && victim_cnt > 0)
    {
        ASSERT(key_bstrs_ptr != NULL);
        ASSERT(value_bstrs_ptr != NULL);
        ASSERT(victim_cnt_ptr != NULL);

        *key_bstrs_ptr = (struct bstring*)malloc(sizeof(struct bstring) * victim_cnt);
        memset(*key_bstrs_ptr, 0, sizeof(struct bstring) * victim_cnt);
        *value_bstrs_ptr = (struct bstring*)malloc(sizeof(struct bstring) * victim_cnt);
        memset(*value_bstrs_ptr, 0, sizeof(struct bstring) * victim_cnt);
    }

    while (curr - seg_data < offset) {
        /* check both offset and n_live_item is because when a segment is expiring
         * and have a slow writer on it, we could observe n_live_item == 0,
         * but we haven't reached offset */
        it = (struct item *) curr;
        if (seg->n_live_item == 0) {
            ASSERT(seg->live_bytes == 0 || seg->live_bytes == 8);

            break;
        }
        if (it->klen == 0 && it->vlen == 0) {
#if defined(CC_ASSERT_PANIC) && defined(DEBUG_MODE)
            scan_hashtable_find_seg(seg->seg_id_non_decr);
#endif
            ASSERT(__atomic_load_n(&seg->n_live_item, __ATOMIC_SEQ_CST) == 0);

            break;
        }

#if defined CC_ASSERT_PANIC || defined CC_ASSERT_LOG
        ASSERT(it->magic == ITEM_MAGIC);
#endif
        ASSERT(it->klen > 0);
        ASSERT(it->vlen >= 0);

        // Siyuan: deep copy the current evicted key-value pair
        if (need_victims)
        {
            ASSERT(victim_idx < victim_cnt);
            (*key_bstrs_ptr)[victim_idx].len = it->klen;
            (*key_bstrs_ptr)[victim_idx].data = (char*)malloc(it->klen);
            memcpy((*key_bstrs_ptr)[victim_idx].data, item_key(it), it->klen);
            (*value_bstrs_ptr)[victim_idx].len = it->vlen;
            (*value_bstrs_ptr)[victim_idx].data = (char*)malloc(it->vlen);
            memcpy((*value_bstrs_ptr)[victim_idx].data, item_val(it), it->vlen);
            victim_idx++;
        }

#if defined DEBUG_MODE
        hashtable_evict(item_key(it), it->klen, seg->seg_id_non_decr,
            curr - seg_data, segcache_ptr);
#else
        hashtable_evict(item_key(it), it->klen, seg->seg_id, curr - seg_data, segcache_ptr); // Siyuan: will decrease seg->n_live_item by 1
#endif

        ASSERT(seg->n_live_item >= 0);
        ASSERT(seg->live_bytes >= 0);

        curr += item_ntotal(it);
    }

    /* at this point, seg->n_live_item could be negative
     * if it is an expired segment and a new item is being wriiten very slowly,
     * and not inserted into hash table */
//    ASSERT(__atomic_load_n(&seg->n_live_item, __ATOMIC_ACQUIRE) >= 0);

    /* all operation up till here does not require refcount to be 0
     * because the data on the segment is not cleared yet,
     * now we are ready to clear the segment data, we need to check refcount.
     * Because we have already locked the segment before removing entries
     * from hashtable, ideally by the time we have removed all hashtable
     * entries, all previous requests on this segment have all finished */
    seg_wait_refcnt(seg_id, segcache_ptr);

    /* optimistic concurrency control:
     * because we didn't wait for refcount before remove hashtable entries
     * it is possible that there are some very slow writers, which finish
     * writing (_item_define) and insert after we clear the hashtable entries,
     * so we need to double check, in most cases, this should not happen */

    bool is_realloc = false; // Siyuan: realloc at most once
    uint32_t extra_victim_cnt = __atomic_load_n(&seg->n_live_item, __ATOMIC_SEQ_CST); // Siyuan: save extra victim_cnt before being changed by hashtable_evict()
    if (__atomic_load_n(&seg->n_live_item, __ATOMIC_SEQ_CST) > 0) {
        INCR(segcache_ptr->seg_metrics, seg_evict_retry);
        /* because we don't know which item is newly written, so we
         * have to remove all items again */
        curr = seg_data;
#if defined CC_ASSERT_PANIC || defined CC_ASSERT_LOG
        curr += sizeof(uint64_t);
#endif
        while (curr - seg_data < offset) {
            it = (struct item *) curr;

            // Siyuan: check if we need to allocate more memory for evicted key-value pairs
            if (need_victims && (victim_idx + extra_victim_cnt > victim_cnt) && !is_realloc)
            {
                // Temporarily save original evicted key-value pair metadata
                uint32_t original_victim_cnt = victim_cnt;
                struct bstring* original_key_bstrs = *key_bstrs_ptr;
                struct bstring* original_value_bstrs = *value_bstrs_ptr;

                // Re-allocate memory for evicted key-value pairs
                victim_cnt = victim_idx + extra_victim_cnt;
                *key_bstrs_ptr = (struct bstring*)malloc(sizeof(struct bstring) * victim_cnt);
                memset(*key_bstrs_ptr, 0, sizeof(struct bstring) * victim_cnt);
                *value_bstrs_ptr = (struct bstring*)malloc(sizeof(struct bstring) * victim_cnt);
                memset(*value_bstrs_ptr, 0, sizeof(struct bstring) * victim_cnt);

                // Shallow copy original evicted key-value pair metadata to new memory
                for (uint32_t i = 0; i < original_victim_cnt; i++)
                {
                    (*key_bstrs_ptr)[i].len = original_key_bstrs[i].len;
                    (*key_bstrs_ptr)[i].data = original_key_bstrs[i].data;
                    (*value_bstrs_ptr)[i].len = original_value_bstrs[i].len;
                    (*value_bstrs_ptr)[i].data = original_value_bstrs[i].data;
                }

                // Release original evicted key-value pair metadata
                free(original_key_bstrs);
                original_key_bstrs = NULL;
                free(original_value_bstrs);
                original_value_bstrs = NULL;

                is_realloc = true;
            }

            // Siyuan: deep copy the current evicted key-value pair
            if (need_victims)
            {
                ASSERT(victim_idx < victim_cnt);
                (*key_bstrs_ptr)[victim_idx].len = it->klen;
                (*key_bstrs_ptr)[victim_idx].data = (char*)malloc(it->klen);
                memcpy((*key_bstrs_ptr)[victim_idx].data, item_key(it), it->klen);
                (*value_bstrs_ptr)[victim_idx].len = it->vlen;
                (*value_bstrs_ptr)[victim_idx].data = (char*)malloc(it->vlen);
                memcpy((*value_bstrs_ptr)[victim_idx].data, item_val(it), it->vlen);
                victim_idx++;
            }

#if defined DEBUG_MODE
            hashtable_evict(item_key(it), it->klen, seg->seg_id_non_decr,
                curr - seg_data);
#else
            hashtable_evict(item_key(it), it->klen, seg->seg_id, curr - seg_data, segcache_ptr);
#endif
            curr += item_ntotal(it);
        }
    }

    // Siyuan: update victim_cnt_ptr
    if (need_victims)
    {
        *victim_cnt_ptr = victim_cnt;
    }

    /* expensive debug commands */
    if (seg->n_live_item != 0) {
        log_warn("removed all items from segment, but %d items left",
            seg->n_live_item);
#if defined(CC_ASSERT_PANIC) && defined(DEBUG_MODE)
        scan_hashtable_find_seg(segcache_ptr->heap_ptr->segs[seg_id].seg_id_non_decr);
#endif
    }

    ASSERT(seg->n_live_item == 0);
    ASSERT(seg->live_bytes == 0 || seg->live_bytes == 8);

    return true;
}

rstatus_i
expire_seg(int32_t seg_id, struct SegCache* segcache_ptr)
{
    // Siyuan: NEVER achieve here under cooperative edge caching!
    ASSERT(false);

    bool success = rm_all_item_on_seg(seg_id, SEG_EXPIRATION, segcache_ptr, false, NULL, NULL, NULL);
    if (!success) {
        return CC_ERROR;
    }

    int status = pthread_mutex_lock(&segcache_ptr->heap_ptr->mtx);
    ASSERT(status == 0);

    seg_add_to_freepool(seg_id, SEG_EXPIRATION, segcache_ptr);

    pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);

    INCR(segcache_ptr->seg_metrics, seg_expire);

    return CC_OK;
}

/**
 * get a seg from free pool,
 *
 * use_reserved: merge-based eviction reserves one seg per thread
 * return the segment id if there are free segment, -1 if not
 */
int32_t
seg_get_from_freepool(bool use_reserved, struct SegCache* segcache_ptr)
{
    int32_t seg_id_ret, next_seg_id;

    int status = pthread_mutex_lock(&segcache_ptr->heap_ptr->mtx);

    if (status != 0) {
        log_warn("fail to lock seg free pool");
        pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);

        return -1;
    }

    if (segcache_ptr->heap_ptr->n_free_seg == 0 ||
        (!use_reserved && segcache_ptr->heap_ptr->n_free_seg <= segcache_ptr->heap_ptr->n_reserved_seg)) {
        pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);

        return -1;
    }

    segcache_ptr->heap_ptr->n_free_seg -= 1;
    ASSERT(segcache_ptr->heap_ptr->n_free_seg >= 0);

    seg_id_ret = segcache_ptr->heap_ptr->free_seg_id;
    ASSERT(seg_id_ret >= 0);

    next_seg_id = segcache_ptr->heap_ptr->segs[seg_id_ret].next_seg_id;
    segcache_ptr->heap_ptr->free_seg_id = next_seg_id;
    if (next_seg_id != -1) {
        segcache_ptr->heap_ptr->segs[next_seg_id].prev_seg_id = -1;
    }

    ASSERT(segcache_ptr->heap_ptr->segs[seg_id_ret].write_offset == 0);

    pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);

    return seg_id_ret;
}

/**
 * add evicted/allocated seg to free pool,
 * caller should grab the heap lock before calling this function
 **/
void
seg_add_to_freepool(int32_t seg_id, enum seg_state_change reason, struct SegCache* segcache_ptr)
{
    ASSERT(pthread_mutex_trylock(&segcache_ptr->heap_ptr->mtx) != 0);

    struct seg *seg = &segcache_ptr->heap_ptr->segs[seg_id];
    seg->next_seg_id = segcache_ptr->heap_ptr->free_seg_id;
    seg->prev_seg_id = -1;
    if (segcache_ptr->heap_ptr->free_seg_id != -1) {
        ASSERT(segcache_ptr->heap_ptr->segs[segcache_ptr->heap_ptr->free_seg_id].prev_seg_id == -1);
        segcache_ptr->heap_ptr->segs[segcache_ptr->heap_ptr->free_seg_id].prev_seg_id = seg_id;
    }
    segcache_ptr->heap_ptr->free_seg_id = seg_id;

    /* we set all free segs as locked to prevent it being evicted
     * before finishing setup */
    ASSERT(seg->evictable == 0);
    seg->accessible = 0;

    /* this is needed to make sure the assert
     * at seg_get_from_freepool do not fail */
    seg->write_offset = 0;
    seg->live_bytes   = 0;

    segcache_ptr->heap_ptr->n_free_seg += 1;

    log_vverb("add %s seg %d to free pool, %d free segs",
        seg_state_change_str[reason], seg_id, segcache_ptr->heap_ptr->n_free_seg);
}

/**
 * get a new segment, search for a free segment in the following order
 * 1. unallocated heap
 * 2. free pool
 * 3. eviction
 **/
int32_t
seg_get_new(struct SegCache* segcache_ptr, bool need_victims, struct bstring** key_bstrs_ptr, struct bstring** value_bstrs_ptr, uint32_t* victim_cnt_ptr)
{
#define MAX_RETRIES 8
    evict_rstatus_e status;
    int32_t         seg_id_ret;
    /* eviction may fail if other threads pick the same seg */
    int             n_retries_left = MAX_RETRIES;

    INCR(segcache_ptr->seg_metrics, seg_get);

    seg_id_ret = seg_get_from_freepool(false, segcache_ptr);

    while (seg_id_ret == -1 && n_retries_left >= 0) {
        /* evict seg */
        if (segcache_ptr->evict_info_ptr->policy == EVICT_MERGE_FIFO) {
            status = seg_merge_evict(&seg_id_ret, segcache_ptr, need_victims, key_bstrs_ptr, value_bstrs_ptr, victim_cnt_ptr);
        } else {
            status = seg_evict(&seg_id_ret, segcache_ptr, need_victims, key_bstrs_ptr, value_bstrs_ptr, victim_cnt_ptr);
        }

        if (status == EVICT_OK) {
            break;
        }

        if (--n_retries_left < MAX_RETRIES) {
            log_warn("retry %d", n_retries_left);

            INCR(segcache_ptr->seg_metrics, seg_evict_retry);
        }
    }

    if (seg_id_ret == -1) {
        INCR(segcache_ptr->seg_metrics, seg_get_ex);
        log_error("unable to get new seg from eviction");

        return -1;
    }

    seg_init(seg_id_ret, segcache_ptr);

    return seg_id_ret;
}

static void
heap_init(struct SegCache* segcache_ptr)
{
    segcache_ptr->heap_ptr->max_nseg  = segcache_ptr->heap_ptr->heap_size / segcache_ptr->heap_ptr->seg_size;
    segcache_ptr->heap_ptr->heap_size = segcache_ptr->heap_ptr->max_nseg * segcache_ptr->heap_ptr->seg_size;
    segcache_ptr->heap_ptr->base      = NULL;

    if (!segcache_ptr->heap_ptr->prealloc) {
        log_crit("%s only support prealloc", SEG_MODULE_NAME);
        exit(EX_CONFIG);
    }
}

static int
setup_heap_mem(struct SegCache* segcache_ptr)
{
    int datapool_fresh = 1;

    segcache_ptr->heap_ptr->pool = datapool_open(segcache_ptr->heap_ptr->poolpath, segcache_ptr->heap_ptr->poolname, segcache_ptr->heap_ptr->heap_size,
        &datapool_fresh, segcache_ptr->heap_ptr->prefault);

    if (segcache_ptr->heap_ptr->pool == NULL || datapool_addr(segcache_ptr->heap_ptr->pool) == NULL) {
        log_crit("create datapool failed: %s - %zu bytes for %" PRIu32 " segs",
            strerror(errno), segcache_ptr->heap_ptr->heap_size, segcache_ptr->heap_ptr->max_nseg);
        exit(EX_CONFIG);
    }

    log_info("pre-allocated %zu bytes for %" PRIu32 " segs", segcache_ptr->heap_ptr->heap_size,
        segcache_ptr->heap_ptr->max_nseg);

    segcache_ptr->heap_ptr->base = datapool_addr(segcache_ptr->heap_ptr->pool);

    return datapool_fresh;
}

static rstatus_i
seg_heap_setup(struct SegCache* segcache_ptr)
{
    heap_init(segcache_ptr);

    int    dram_fresh = 1;
    size_t seg_hdr_sz = SEG_HDR_SIZE * segcache_ptr->heap_ptr->max_nseg;

    dram_fresh = setup_heap_mem(segcache_ptr);
    pthread_mutex_init(&segcache_ptr->heap_ptr->mtx, NULL);

    segcache_ptr->heap_ptr->segs = cc_zalloc(seg_hdr_sz);

    if (!dram_fresh) {
        /* TODO(jason): recover */
        ;
    }
    else {
        pthread_mutex_lock(&segcache_ptr->heap_ptr->mtx);
        segcache_ptr->heap_ptr->n_free_seg = 0;
        for (int32_t i = segcache_ptr->heap_ptr->max_nseg - 1; i >= 0; i--) {
            segcache_ptr->heap_ptr->segs[i].seg_id          = i;
#ifdef DEBUG_MODE
            segcache_ptr->heap_ptr->segs[i].seg_id_non_decr = i;
#endif
            segcache_ptr->heap_ptr->segs[i].evictable       = 0;
            segcache_ptr->heap_ptr->segs[i].accessible      = 0;

            seg_add_to_freepool(i, SEG_ALLOCATION, segcache_ptr);
        }
        pthread_mutex_unlock(&segcache_ptr->heap_ptr->mtx);
    }

    return CC_OK;
}

void
seg_teardown(struct SegCache* segcache_ptr)
{
    log_info("tear down the %s module", SEG_MODULE_NAME);

    segcache_ptr->stop = true;

    if (!segcache_ptr->disable_expiration) // Siyuan: wait for background thread of expiration if enabled (always disabled for cooperative edge caching framework, as we do NOT target timeout-based cache)
    {
        // Siyuan: should NOT arrive here!!!
        printf("ERROR: should NOT arrive here for expiration in seg_teardown()!!!");
        exit(1);

        pthread_join(segcache_ptr->bg_tid, NULL);
    }

    if (!segcache_ptr->seg_initialized) {
        log_warn("%s has never been set up", SEG_MODULE_NAME);
        return;
    }

    hashtable_teardown(segcache_ptr);

    segevict_teardown(segcache_ptr);
    ttl_bucket_teardown();

    segcache_ptr->seg_metrics = NULL;

    segcache_ptr->flush_at        = -1;
    segcache_ptr->seg_initialized = false;
}

void
seg_setup(seg_options_st *options, seg_metrics_st *metrics, struct SegCache* segcache_ptr)
{
    log_info("set up the %s module", SEG_MODULE_NAME);

    if (segcache_ptr->seg_initialized) {
        log_warn("%s has already been set up, re-creating", SEG_MODULE_NAME);
        seg_teardown(segcache_ptr);
    }

    segcache_ptr->seg_metrics = metrics;

    if (options == NULL) {
        log_crit("no option is provided for seg initialization");
        exit(EX_CONFIG);
    }

    segcache_ptr->flush_at = -1;
    segcache_ptr->stop     = false;

    segcache_ptr->seg_options = options;
    segcache_ptr->n_thread    = option_uint(&segcache_ptr->seg_options->seg_n_thread);

    segcache_ptr->heap_ptr->seg_size  = option_uint(&segcache_ptr->seg_options->seg_size);
    segcache_ptr->heap_ptr->heap_size = option_uint(&segcache_ptr->seg_options->heap_mem);
    log_verb("cache size %" PRIu64, segcache_ptr->heap_ptr->heap_size);

    segcache_ptr->heap_ptr->free_seg_id = -1;
    segcache_ptr->heap_ptr->prealloc    = option_bool(&segcache_ptr->seg_options->seg_prealloc);
    segcache_ptr->heap_ptr->prefault    = option_bool(&segcache_ptr->seg_options->datapool_prefault);

    segcache_ptr->heap_ptr->poolpath = option_str(&segcache_ptr->seg_options->datapool_path);
    segcache_ptr->heap_ptr->poolname = option_str(&segcache_ptr->seg_options->datapool_name);

    segcache_ptr->heap_ptr->n_reserved_seg = 0;

    segcache_ptr->use_cas = option_bool(&segcache_ptr->seg_options->seg_use_cas);

    hashtable_setup(option_uint(&segcache_ptr->seg_options->hash_power), segcache_ptr);

    if (seg_heap_setup(segcache_ptr) != CC_OK) {
        log_crit("Could not setup seg heap info");
        goto error;
    }

    ttl_bucket_setup(segcache_ptr);

    segcache_ptr->evict_info_ptr->merge_opt.seg_n_merge     =
        option_uint(&segcache_ptr->seg_options->seg_n_merge);
    segcache_ptr->evict_info_ptr->merge_opt.seg_n_max_merge =
        option_uint(&segcache_ptr->seg_options->seg_n_max_merge);
    segevict_setup(option_uint(&options->seg_evict_opt),
        option_uint(&segcache_ptr->seg_options->seg_mature_time), segcache_ptr);
    if (segcache_ptr->evict_info_ptr->policy == EVICT_MERGE_FIFO) {
        segcache_ptr->heap_ptr->n_reserved_seg = segcache_ptr->n_thread;
    }

    if (!segcache_ptr->disable_expiration) // Siyuan: start background thread for expiration if enabled (always disabled for cooperative edge caching framework, as we do NOT target timeout-based cache)
    {
        // Siyuan: should NOT arrive here!!!
        printf("ERROR: should NOT arrive here for expiration in seg_setup()!!!");
        exit(1);

        start_background_thread(NULL, segcache_ptr);
    }

    segcache_ptr->seg_initialized = true;

    log_info("Seg header size: %d, item header size: %d, eviction algorithm %s",
        SEG_HDR_SIZE, ITEM_HDR_SIZE, eviction_policy_names[segcache_ptr->evict_info_ptr->policy]);

    return;

    error:
    seg_teardown(segcache_ptr);
    exit(EX_CONFIG);
}

// Siyuan: allow forward declaration to break circular dependency

uint8_t *
get_seg_data_start(int32_t seg_id, struct SegCache* segcache_ptr)
{
    return segcache_ptr->heap_ptr->base + segcache_ptr->heap_ptr->seg_size * seg_id;
}