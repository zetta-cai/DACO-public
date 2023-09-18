/*
 * Common header shared by different modules for COVERED.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef COVERED_COMMON_HEADER_H
#define COVERED_COMMON_HEADER_H

#include <cstdint> // uint32_t, uint64_t

namespace covered
{
    // TODO: Tune per-variable size later
    typedef uint32_t GroupId;
    typedef uint32_t Frequency;
    typedef uint32_t ObjectSize;
    typedef uint32_t ObjectCnt;
    
    typedef float Popularity;
    typedef float Weight; // Weight parameters to calculate Reward
    typedef float Reward; // Local reward for local cached objects, and conceptual global reward to define admission benefit (increased global reward) and global eviction cost (decreased global reward)
    typedef Reward DeltaReward; // Local admission benefit for local uncached metadata, max global admission benefit for aggregated uncached popularity, global admission benefit and global eviction cost to define PlacementGain (global admission benefit - global eviction cost) for trade-off-aware cache placement and eviction

    // Max keycnt per group for local cached/uncached objects
    #define COVERED_PERGROUP_MAXKEYCNT 10 // At most 10 keys per group for local cached/uncached objects
}

#endif