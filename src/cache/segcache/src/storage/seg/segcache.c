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

    // Siyuan: whether to disable timeout-based expiration
    segcache_ptr->disable_expiration = true;

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

uint64_t get_segcache_size_bytes(struct SegCache* segcache_ptr)
{
    uint64_t size_bytes = 0;

    // Refer to src/cache/segcache/src/storage/seg/hashtable.c
    uint64_t hashtable_size = 0;
    // NOTE: object-level frequency and recency are encoded into uint64_t iteminfo as well as <tag, segment id, offset> for key and item address
    int valid_iteminfo_cnt = 0; // # of valid iteminfos in hash table (NOT include unused iteminfos)
    int head_bucket_cnt = 0; // # of head buckets for hash chains
    hashtable_stat(&valid_iteminfo_cnt, &head_bucket_cnt, segcache_ptr);
    hashtable_size = (valid_iteminfo_cnt + head_bucket_cnt) * sizeof(uint64_t);

    uint64_t ttl_metadata_size = 0;
    ttl_metadata_size += sizeof(struct ttl_bucket) * MAX_N_TTL_BUCKET; // ttl_buckets
    ttl_metadata_size += sizeof(seg_perttl_metrics_st) * MAX_N_TTL_BUCKET; // perttl

    // Refer to src/cache/segcache/src/storage/seg/seg.c
    // NOTE: segment-level statistics are stored in segment headers
    uint64_t seg_metadata_size = SEG_HDR_SIZE * segcache_ptr->heap_ptr->max_nseg;

    // Refer to src/cache/segcache/src/storage/seg/seg.h
    // NOTE: seg_data_size includes deleted bytes, as they have not been released for new items due to log-structured design (can ONLY be allocated for new items after eviction)
    uint64_t seg_data_size = 0;
    for (uint32_t i = 0; i < segcache_ptr->heap_ptr->max_nseg; i++)
    {
        seg_data_size += segcache_ptr->heap_ptr->segs[i].total_bytes;
    }

    size_bytes = hashtable_size + ttl_metadata_size + seg_metadata_size + seg_data_size;

    return size_bytes;
}