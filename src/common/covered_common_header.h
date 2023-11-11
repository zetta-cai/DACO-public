/*
 * Common header shared by different modules for COVERED.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef COVERED_COMMON_HEADER_H
#define COVERED_COMMON_HEADER_H

// Used in src/cache/covered_local_cache.c and src/cache/covered/*
// NOTE: we track key-level accurate object size by default to avoid affecting cache management decisions -> although existing studies (e.g., Segcache) use group-level object size, they may have similar object sizes, while edge caching has significantly different object sizes, which will affect cache management decisions
// -> If defined, we track accurate object size in key-level metadata (by default)
// -> If not defined, we track approximate object size in group-level metadata (NOT recommend due to inaccurate object size and less-effective cache management)
#define ENABLE_TRACK_PERKEY_OBJSIZE

// Used in src/cache/covered_local_cache.c and src/cache/covered/*
// (OBSOLETE due to waiting for 2nd request to evict one-hit-wonders when cache is close to full) NOTE: use max slab size as conservative object size to track uncached key in local uncached metadata for getreq with cache miss if NOT tracked instead of waiting for the second getreq to trigger cache management, which will slow down admission rate especially for the beginning of warmup phase
// -> If defined, we use conservative local uncached popularity for the first cache miss for each uncached object to fill up cache quickly
// -> If not defined, we wait for at least the second cache miss for each uncached object to trigger cache management
//#define ENABLE_CONSERVATIVE_UNCACHED_POP

// Used in src/cache/covered_local_cache.c, src/cache/covered/*, and src/core/popularity/collected_popularity.c
// (OBSOLETE due to still waiting for at least second request especially under scan patterns) NOTE: maintain an auxiliary data cache for the values of local uncached objects tracked by local uncached metadata
// -> If defined, we track the values in object-level metadata of local uncached metadata (constrained by local uncached metadata capacity limitation)
// -> If not defined, we access neighbor/cloud again for at least the second cache miss of each uncached object for trade-off-aware placement calculation
//#define ENABLE_AUXILIARY_DATA_CACHE

// NOTE: ENABLE_CONSERVATIVE_UNCACHED_POP and ENABLE_AUXILIARY_DATA_CACHE cannot defined simultaneously, which will admit max slab size into local edge cache for getreq w/ first cache miss
// #ifdef ENABLE_AUXILIARY_DATA_CACHE
// #undef ENABLE_CONSERVATIVE_UNCACHED_POP
// #endif
// #ifdef ENABLE_CONSERVATIVE_UNCACHED_POP
// #undef ENABLE_AUXILIARY_DATA_CACHE
// #endif

// Used in src/cache/covered/*
// (OBSOLETE due to one-hit-wonder is NOT issue of slow warmup and freq/objsize is even better) Use zero popularity values with MRU for one-hit-wonders to quickly evict them
// -> If defined, we use MRU policy for zero-popularity one-hit-wonders
// -> If not defined, we use LRU policy for equal popularity values (use non-zero freq/objsize as popularity for one-hit-wonders)
//#define ENABLE_MRU_FOR_ONE_HIT_WONDERS

// Used in
// NOTE: we could ONLY trigger fast-path single-placement calculation for local/remote directory lookup if key is NOT tracked by sender local uncached metadata -> NO need for local/remote dirinfo eviction, as the object is just evicted from local edge cache instead of NO objsize due to (possibly) the first cache miss w/o objsize; also NO need for local/remote acquire writelock and release writelock, as the object has the latest objsize (provided by the put/del request) instead of NO objsize due to (possibly) the first cache miss w/o objsize
// NOTE: we use single placement calculation, as aggregated popularity info will be stale after sender edge node fetches value from cloud/neighbor, but local uncached popularity is always latest, which can be used for trade-off-aware placement calculation for the sender edge node!!!
// NOTE: although fast path uses approximate global admission policy and cooperation-aware cache placement, it is just used for fast cache warmup -> after cache is warmed up (i.e., most requests are NOT the first cache miss of the objects and hence with object size information), we can rely on the beacon edge node for global admission and cache placement with the latest global view
// -> If defined, we use fast path for single-placement calculation triggered by getrsp with the first cache miss to fix slow warmup issue
// -> If not defined, we only perform normal trade-off-aware cache placement in beacon edge node for at lest 2nd get request of each uncached object
#define ENABLE_FAST_PATH_PLACEMENT

#include <cstdint> // uint32_t, uint64_t

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
}

#endif