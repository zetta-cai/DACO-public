/*
 * Common header shared by different modules for COVERED.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef COVERED_COMMON_HEADER_H
#define COVERED_COMMON_HEADER_H

// Used in src/cache/covered/*
// NOTE: we track key-level accurate object size by default to avoid affecting cache management decisions -> although existing studies (e.g., Segcache) use group-level object size, they may have similar object sizes, while edge caching has significantly different object sizes, which will affect cache management decisions
// -> If defined, we track accurate object size in key-level metadata
// -> If not defined, we track approximate object size in group-level metadata
#define TRACK_PERKEY_OBJSIZE

// TMPDEBUG231108
#define ENABLE_APPROX_UNCACHED_POP

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

    // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)
    typedef uint64_t SeqNum; // Sequence number for victim synchronization

    // Max keycnt per group for local cached/uncached objects
    #define COVERED_PERGROUP_MAXKEYCNT 10 // At most 10 keys per group for local cached/uncached objects
}

#endif