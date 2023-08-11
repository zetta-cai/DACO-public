#include "storage/seg/segcache.h"

void initializeSegcache(struct SegCache* segcache_ptr)
{
    // From src/time/time.c
    segcache_ptr->time_type = TIME_UNIX; // NOTE: TIME_UNIX = 0 is an anonymous enum defined in src/time/time.h

    // From src/storage/seg/hash_table.c
    segcache_ptr->hash_table_initialized = false;

    // From src/storage/seg/seg.c
    segcache_ptr->heap_ptr = new struct seg_heapinfo;
    segcache_ptr->ttl_buckets = new struct ttl_bucket[MAX_N_TTL_BUCKET];
    segcache_ptr->seg_initialized = false;
    segcache_ptr->seg_metrics = NULL;
    segcache_ptr->seg_options = NULL;
    segcache_ptr->perttl = new seg_perttl_metrics_st[MAX_N_TTL_BUCKET];
    segcache_ptr->flush_at = -1;
    segcache_ptr->use_cas = false;
    segcache_ptr->n_thread = 1;
    segcache_ptr->stop = false;

    return;
}

void releaseSegCache(struct SegCache* segcache_ptr)
{
    // From src/storage/seg/seg.c
    delete segcache_ptr->heap_ptr;
    segcache_ptr->heap_ptr = NULL;
    delete[] segcache_ptr->ttl_buckets;
    segcache_ptr->ttl_buckets = NULL;
    delete[] segcache_ptr->perttl;
    segcache_ptr->perttl = NULL;
}