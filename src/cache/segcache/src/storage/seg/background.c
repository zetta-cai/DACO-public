
#include <assert.h>

#include "background.h"
#include "item.h"
#include "seg.h"
#include "ttlbucket.h"

#include "cc_debug.h"
#include "time/cc_wheel.h"

#include <pthread.h>
#include <sysexits.h>
#include <time.h>

// Siyuan: encapsulate global variables
// extern volatile bool        stop;
// extern volatile proc_time_i flush_at;
// extern pthread_t            bg_tid;
// extern struct ttl_bucket    ttl_buckets[MAX_N_TTL_BUCKET];

// Siyuan: add to pass struct SegCache
struct background_param_t
{
    void* arg;
    struct SegCache* segcache_ptr;
};

static inline void initialize_background_param(struct background_param_t* background_param_ptr)
{
    background_param_ptr->arg = NULL;
    background_param_ptr->segcache_ptr = NULL;
    return;
}

static void
check_seg_expire(struct SegCache* segcache_ptr)
{
    rstatus_i   status;
    struct seg  *seg;
    int32_t     seg_id, next_seg_id;
    for (int i = 0; i < MAX_N_TTL_BUCKET; i++) {
        seg_id = segcache_ptr->ttl_buckets[i].first_seg_id;
        if (seg_id == -1) {
            /* no object of this TTL */
            continue;
        }

        seg = &segcache_ptr->heap_ptr->segs[seg_id];
        /* curr_sec - 2 to avoid a slow client is still writing to
         * the expiring segment  */
        while (seg->create_at + seg->ttl < time_proc_sec(segcache_ptr) - 2 ||
            seg->create_at < segcache_ptr->flush_at) {
            log_debug("expire seg %"PRId32 ", create at %"PRId32 ", ttl %"PRId32
            ", flushed at %"PRId32, seg_id, seg->create_at, seg->ttl, segcache_ptr->flush_at);

            next_seg_id = seg->next_seg_id;

            status = expire_seg(seg_id, segcache_ptr);
            if (status != CC_OK) {
                log_error("error removing expired seg %d", seg_id);
            }

            if (next_seg_id == -1) {
                break;
            }

            seg_id = next_seg_id;
            seg    = &segcache_ptr->heap_ptr->segs[seg_id];
        }
    }
}

static void *
background_main(void *background_param_ptr)
{
#ifdef __APPLE__
    pthread_setname_np("segBg");
#else
    pthread_setname_np(pthread_self(), "segBg");
#endif

    log_info("Segcache background thread started");

    struct background_param_t* tmp_background_param_ptr = (struct background_param_t*) background_param_ptr;

    while (!tmp_background_param_ptr->segcache_ptr->stop) {
        check_seg_expire(tmp_background_param_ptr->segcache_ptr);

        // do we want to enable background eviction?
        // merge_based_eviction();

        usleep(200000);
    }

    log_info("seg background thread stopped");
    return NULL;
}

void
start_background_thread(void *arg, struct SegCache* segcache_ptr)
{
    assert(!segcache_ptr->disable_expiration); // Siyuan: we MUST NOT disable expiration if start background thread

    struct background_param_t background_param;
    initialize_background_param(&background_param);
    background_param.arg = arg;
    background_param.segcache_ptr = segcache_ptr;

    int ret = pthread_create(&segcache_ptr->bg_tid, NULL, background_main, &background_param);
    if (ret != 0) {
        log_crit("pthread create failed for background thread: %s",
            strerror(ret));
        exit(EX_OSERR);
    }
}

