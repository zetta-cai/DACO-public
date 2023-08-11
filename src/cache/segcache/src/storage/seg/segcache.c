#include "storage/seg/segcache.h"

void initialize_segcache(struct SegCache* segcache_ptr)
{
    // From src/time/time.c
    segcache_ptr->time_type = TIME_UNIX; // NOTE: TIME_UNIX = 0 is an anonymous enum defined in src/time/time.h

    // From src/storage/seg/hash_table.c
    segcache_ptr->hash_table_initialized = false;

    // From src/storage/seg/segevict.c
    segcache_ptr->evict_info_ptr = (struct seg_evict_info*)malloc(sizeof(struct seg_evict_info));
    memset(segcache_ptr->evict_info_ptr, 0, sizeof(struct seg_evict_info));

    // From src/storage/seg/segmerge.c
    segcache_ptr->seg_evict_seg_cnt = 0;
    segcache_ptr->seg_evict_seg_sum = 0;

    // From src/storage/seg/seg.c
    segcache_ptr->heap_ptr = (struct seg_heapinfo*)malloc(sizeof(struct seg_heapinfo));
    memset(segcache_ptr->heap_ptr, 0, sizeof(struct seg_heapinfo));
    segcache_ptr->ttl_buckets = (struct ttl_bucket*)malloc(sizeof(struct ttl_bucket) * MAX_N_TTL_BUCKET);
    memset(segcache_ptr->ttl_buckets, 0, sizeof(struct ttl_bucket) * MAX_N_TTL_BUCKET);
    segcache_ptr->seg_initialized = false;
    segcache_ptr->seg_metrics = NULL;
    segcache_ptr->seg_options = NULL;
    segcache_ptr->perttl = (seg_perttl_metrics_st*)malloc(sizeof(seg_perttl_metrics_st) * MAX_N_TTL_BUCKET);
    memset(segcache_ptr->perttl, 0, sizeof(seg_perttl_metrics_st) * MAX_N_TTL_BUCKET);
    segcache_ptr->flush_at = -1;
    segcache_ptr->use_cas = false;
    segcache_ptr->n_thread = 1;
    segcache_ptr->stop = false;

    return;
}

void release_segcache(struct SegCache* segcache_ptr)
{
    // From src/storage/seg/segevict.c
    free(segcache_ptr->evict_info_ptr);
    segcache_ptr->evict_info_ptr = NULL;

    // From src/storage/seg/seg.c
    free(segcache_ptr->heap_ptr);
    segcache_ptr->heap_ptr = NULL;
    free(segcache_ptr->ttl_buckets);
    segcache_ptr->ttl_buckets = NULL;
    free(segcache_ptr->perttl);
    segcache_ptr->perttl = NULL;
}