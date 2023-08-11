/*
 * SegCache: encapsulate non-const global variables (including both and non-static) required by segcache.
 *
 * NOTE: we do NOT direclty use __thread or thread_local to declare global variables, as it cannot support multiple instances within the same thread, and NOT memory efficiency due to creating a new instance for each thread even if the thread does NOT need the instance.
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#pragma once

#include <time/cc_timer.h>

#include <time/time.h>
#include <storage/seg/hashtable.h>

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
    uint8_t time_type = TIME_UNIX;

    // From src/storage/seg/hash_table.c
    struct hash_table hash_table;
    bool hash_table_initialized = false;
    //static __thread __uint128_t g_lehmer64_state = 1; // NO need to encapsulate thread-local variables

    // From src/storage/seg/ttlbucket.c
    //__thread int32_t local_last_seg[MAX_N_TTL_BUCKET] = {0}; // NO need to encapsulate thread-local variables

    // From src/storage/seg/segevict.c
    bool segevict_initialized;
    struct seg_evict_info evict_info;

    // From src/storage/seg/seg.c
    struct seg_heapinfo heap; /* info of all allocated segs */
    struct ttl_bucket ttl_buckets[MAX_N_TTL_BUCKET];
    bool seg_initialized = false;
    seg_metrics_st *seg_metrics = NULL;
    seg_options_st *seg_options = NULL;
    seg_perttl_metrics_st perttl[MAX_N_TTL_BUCKET];
    proc_time_i flush_at = -1;
    bool use_cas = false;
    pthread_t bg_tid;
    int n_thread = 1;
    volatile bool stop = false;
};