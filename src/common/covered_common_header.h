/*
 * Common header shared by different modules for COVERED.
 *
 * NOTE: do NOT change the following macros if you do NOT understand what you are doing!!!
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef COVERED_COMMON_HEADER_H
#define COVERED_COMMON_HEADER_H

// Used in src/cache/covered_local_cache.c and src/cache/covered/*
// NOTE: we track key-level accurate object size by default to avoid affecting cache management decisions -> although existing studies (e.g., Segcache) use group-level object size, they may have similar object sizes, while edge caching has significantly different object sizes, which will affect cache management decisions
// ---> If defined, we track accurate object size in key-level metadata (by default)
// ---> If not defined, we track approximate object size in group-level metadata (NOT recommend due to inaccurate object size and less-effective cache management)
#define ENABLE_TRACK_PERKEY_OBJSIZE

// Used in src/core/covered_cache_mananger.c, src/core/popularity_aggregator.c, src/edge/beacon_server/covered_beacon_server.c, and src/edge/cache_server/covered_cache_server_worker.c
// NOTEs:
// (i) We could ONLY trigger fast-path single-placement calculation for local/remote directory lookup if key is NOT tracked by sender local uncached metadata -> NO need for local/remote dirinfo eviction, as the object is just evicted from local edge cache instead of NO objsize due to (possibly) the first cache miss w/o objsize; also NO need for local/remote acquire writelock and release writelock, as the object has the latest objsize (provided by the put/del request) instead of NO objsize due to (possibly) the first cache miss w/o objsize
// (ii) We use single placement calculation, as aggregated popularity info will be stale after sender edge node fetches value from cloud/neighbor, but local uncached popularity is always latest, which can be used for trade-off-aware placement calculation for the sender edge node!!!
// (iii) although fast path uses approximate global admission policy and cooperation-aware cache placement, it is just used for fast cache warmup -> after cache is warmed up (i.e., most requests are NOT the first cache miss of the objects and hence with object size information), we can rely on the beacon edge node for global admission and cache placement with the latest global view
// ---> If defined, we use fast path for single-placement calculation triggered by getrsp with the first cache miss to fix slow warmup issue (by default)
// ---> If not defined, we only perform normal trade-off-aware cache placement in beacon edge node for at lest 2nd get request of each uncached object (NOT recommend due to slow cache warmup under large edge/dataset scale)
#define ENABLE_FAST_PATH_PLACEMENT

// Used in src/edge/edge_wrapper.c (reward calculation for local reward of local cached/uncached objects, or normal/fast-path cache placement)
// NOTE: always enforce duplication avoidance no matter cache is not full or already warmed up
// ---> If defined, we NEVER admit duplicate cache copies of an object as long as it has been cached by an edge node (NOT recommend due to significant degradation on local hit ratio; ONLY used for debugging)
// ---> If not defined, manage cooperative edge caching based on the trade-off between admission benefit and eviction cost, i.e., trade-off between local hit ratio and cooperative hit ratio (by default)
//#define ENABLE_COMPLETE_DUPLICATION_AVOIDANCE_FOR_DEBUG

// NOTE: always use beaccon-baesd cached metadata update to track is_neighbor_cached flag for each cached object
// ---> Issue: unnecessary duplicate cache copies (small admission benefits) admited during cache warmup (eviction cost = 0 due to not-full cache, or extremely small eviction cost due to one-hit-wonders) CANNOT be evicted after cache is filled up, as local edge node does NOT know if any neighbor also caches this object, which over-estimates the local reward of duplicately cached objects
// ---> Beacon-based cached metadata update: initialize is_neighbor_cached based on local/remote directory admission response before each local cache admission; enable/disable is_neighbor_cached at the first/last cached edge node when the cached object becomes non-single/single cache copy after directory admission/eviction
// ---> Limited extra communication overhead: beacon node ONLY piggybacks a flag for each newly-admited object ONCE at admission, and ONLY notify the first/last cache copy when the object becomes non-single/single edge cached

// Used in src/cli/evaluator_cli.c and src/benchmark/evaluator_wrapper.c
// NOTE: we fix the same number of requests to warmup all methods with fairness.
// ---> If defined, we do NOT use any other condition to finish warmup phase.
// ---> If not defined, we also monitor stability of global object hit ratio within warmup maximum duration (in units of seconds) after achieving warmup reqcnt if necessary (i.e., w/ remaining time).
//#define ENABLE_WARMUP_MAX_DURATION

// Used in src/cache/covered_local_cache.*
// ---> If defined, use a small LRU cache (src/cache/covered/local_uncached_lru.*) to decide whether to update local uncached metadata (priority queue of access statistics of uncached objects).
// ---> If not defined, always evict the uncached object with the smallest priority in local uncached metadata for untracked objects.
#define ENABLE_SMALL_LRU_CACHE

#include <cstdint> // uint32_t, uint64_t
#include <assert.h>

#include "common/util.h"

namespace covered
{
    // TODO: Tune per-variable size later
    typedef uint32_t GroupId;
    typedef uint32_t Frequency;
    typedef uint32_t ObjectSize;
    typedef float AvgObjectSize;
    typedef uint32_t ObjectCnt;
    
    typedef float Popularity;
    typedef float Weight; // Weight parameters to calculate Reward
    typedef float Reward; // Local reward for local cached objects, and conceptual global reward to define admission benefit (increased global reward) and eviction cost (decreased global reward)
    typedef Reward DeltaReward; // Approximate admission benefit for local uncached metadata, max admission benefit for aggregated uncached popularity, admission benefit and eviction cost to define PlacementGain (admission benefit - eviction cost) for trade-off-aware cache placement and eviction

    #define MIN_ADMISSION_BENEFIT 0.0

    // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)
    typedef uint64_t SeqNum; // Sequence number for victim synchronization

    // Max keycnt per group for local cached/uncached objects
    #define COVERED_PERGROUP_MAXKEYCNT 10 // At most 10 keys per group for local cached/uncached objects

    inline Popularity calculatePopularity(const Frequency& frequency, const ObjectSize& object_size)
    {
        // (OBSOLETE: zero-reward for one-hit-wonders will mis-evict hot keys) Set popularity as zero for zero-reward of one-hit-wonders to quickly evict them
        // if (frequency <= 1)
        // {
        //     return 0;
        // }

        ObjectSize tmp_object_size = object_size;

        // (OBSOLETE: we CANNOT directly use recency_index, as each object has an recency_index of 1 when updating popularity and we will NOT update recency info of all objects for each cache hit/miss)
        //uint32_t recency_index = std::distance(perkey_metadata_list_.begin(), perkey_metadata_const_iter) + 1;
        //tmp_object_size *= recency_index;

        // NOTE: Here we use a simple approach to calculate popularity
        Popularity popularity = 0.0;

        if (tmp_object_size == 0) // Zero object size due to delreqs or approximate value sizes in local uncached metadata
        {
            #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
            assert(false); // TMPDEBUG
            tmp_object_size = 1; // Give the largest possible popularity for delreqs due to zero space usage for deleted value
            #else
            popularity = 0; // Set popularity as zero to avoid mis-admiting the uncached object with unknow object size if w/ approximate value sizes
            return popularity;
            #endif
        }
        
        //ObjectSize tmp_objsize_kb = B2KB(tmp_object_size);
        popularity = Util::popularityDivide(static_cast<Popularity>(frequency), static_cast<Popularity>(tmp_object_size)); // # of cache accesses per space unit (similar as LHD)

        return popularity;
    }
}

#endif