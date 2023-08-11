/*
 * SegCache: encapsulate non-const global variables (including both and non-static) required by segcache.
 *
 * NOTE: we do NOT direclty use __thread or thread_local to declare global variables, as it cannot support multiple instances within the same thread, and NOT memory efficiency due to creating a new instance for each thread even if the thread does NOT need the instance.
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#pragma once

#include <stdint.h> // uint8_t

#include <time/cc_timer.h>

struct SegCache;

#include "time/time.h"
#include "storage/seg/hashtable.h"
#include "storage/seg/segevict.h"
#include "storage/seg/seg.h"
#include "storage/seg/ttlbucket.h"
#include "storage/seg/hashtable.h"

struct SegCache
{
    // From src/time/time.c
    time_t time_start;
    proc_time_i proc_sec;
    proc_time_fine_i proc_ms;
    proc_time_fine_i proc_us;
    proc_time_fine_i proc_ns;
    // OS_LINUX will be set by src/cache/segcache/CMakeLists.txt for struct duration
    struct duration start;
    struct duration proc_snapshot;
    uint8_t time_type;

    // From src/storage/seg/hash_table.c
    struct hash_table hash_table;
    bool hash_table_initialized;
    //static __thread __uint128_t g_lehmer64_state = 1; // NO need to encapsulate thread-local variables

    // From src/storage/seg/ttlbucket.c
    //__thread int32_t local_last_seg[MAX_N_TTL_BUCKET] = {0}; // NO need to encapsulate thread-local variables

    // From src/storage/seg/segevict.c
    bool segevict_initialized;
    struct seg_evict_info* evict_info_ptr;

    // From src/storage/seg/segmerge.c
    uint64_t seg_evict_seg_cnt; 
    uint64_t seg_evict_seg_sum;

    // From src/storage/seg/seg.c
    struct seg_heapinfo* heap_ptr; /* info of all allocated segs */
    struct ttl_bucket* ttl_buckets;;
    bool seg_initialized;
    seg_metrics_st *seg_metrics;
    seg_options_st *seg_options;
    seg_perttl_metrics_st* perttl;
    proc_time_i flush_at;
    bool use_cas;
    pthread_t bg_tid;
    int n_thread;
    volatile bool stop;
};

void initialize_segcache(struct SegCache* segcache_ptr);
void release_segcache(struct SegCache* segcache_ptr);