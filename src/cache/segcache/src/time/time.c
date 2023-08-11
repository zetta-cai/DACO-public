#include "time/time.h"

#include <cc_debug.h>
#include <time/cc_timer.h>

#include <sysexits.h>

// Siyuan: encapsulate global variables
// time_t time_start;
// proc_time_i proc_sec;
// proc_time_fine_i proc_ms;
// proc_time_fine_i proc_us;
// proc_time_fine_i proc_ns;

// Siyuan: encapsulate global variables
//static struct duration start;
// static struct duration proc_snapshot;

// Siyuan: encapsulate global variables
//uint8_t time_type = TIME_UNIX;

void
time_update(struct SegCache* segcache_ptr)
{
    duration_snapshot(&segcache_ptr->proc_snapshot, &segcache_ptr->start);

    __atomic_store_n(&segcache_ptr->proc_sec, (proc_time_i)duration_sec(&segcache_ptr->proc_snapshot),
            __ATOMIC_RELAXED);
    __atomic_store_n(&segcache_ptr->proc_ms, (proc_time_fine_i)duration_ms(&segcache_ptr->proc_snapshot),
            __ATOMIC_RELAXED);
    __atomic_store_n(&segcache_ptr->proc_us, (proc_time_fine_i)duration_us(&segcache_ptr->proc_snapshot),
            __ATOMIC_RELAXED);
    __atomic_store_n(&segcache_ptr->proc_ns, (proc_time_fine_i)duration_ns(&segcache_ptr->proc_snapshot),
            __ATOMIC_RELAXED);
}

void
time_setup(time_options_st *options, struct SegCache* segcache_ptr)
{
    if (options != NULL) {
        segcache_ptr->time_type = option_uint(&options->time_type);
    }

    segcache_ptr->time_start = time(NULL);
    duration_start(&segcache_ptr->start);
    time_update(segcache_ptr);

    log_info("timer started at %"PRIu64, (uint64_t)segcache_ptr->time_start);

    if (segcache_ptr->time_type >= TIME_SENTINEL) {
        exit(EX_CONFIG);
    }
}

void
time_teardown(struct SegCache* segcache_ptr)
{
    duration_reset(&segcache_ptr->start);
    duration_reset(&segcache_ptr->proc_snapshot);

    log_info("timer ended at %"PRIu64, (uint64_t)time(NULL));
}
