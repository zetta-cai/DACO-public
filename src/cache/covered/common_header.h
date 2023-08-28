/*
 * Common header shared by different files of COVERED local cache.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

#include "cache/cachelib/CacheAllocator-inl.h" // LruAllocator

namespace covered
{
    // Forward declaration
    class CoveredLocalCache;

    typedef LruAllocator CachelibLruCache; // LRU2Q cache policy
    typedef CachelibLruCache::Config LruCacheConfig;
    typedef CachelibLruCache::ReadHandle LruCacheReadHandle;
    typedef CachelibLruCache::Item LruCacheItem;

    // TODO: Tune per-variable size later
    typedef uint32_t GroupId;
    typedef uint32_t Frequency;
    typedef uint32_t ObjectSize;
    typedef uint32_t ObjectCnt;
    
    typedef float Popularity;

    // Max keycnt per group for local cached/uncached objects
    #define COVERED_PERGROUP_MAXKEYCNT 10 // At most 10 keys per group for local cached/uncached objects

    // Max memory usage for local uncached objects (min between 1% of total cache capacity and 1 MiB)
    #define COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO 0.01 // At most 1% of total cache capacity for local uncached objects
    #define COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_BYTES MB2B(1) // At most 1 MiB for local uncached objects
}

#endif