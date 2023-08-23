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

    typedef LruAllocator LruCache; // LRU2Q cache policy
    typedef LruCache::Config LruCacheConfig;
    typedef LruCache::ReadHandle LruCacheReadHandle;
    typedef LruCache::Item LruCacheItem;

    typedef uint32_t GroupId;
    typedef float Popularity;

    // Max keycnt per group for local cached/uncached objects
    #define COVERED_PERGROUP_MAXKEYCNT 10 // At most 10 keys per group for local cached/uncached objects

    // MAx keycnt of all groups for local uncached objects (limit memory usage for local uncached objects)
    #define COVERED_LOCAL_UNCACHED_MAX_TRACKCNT 1000 // At most 1000 keys in total for local uncached objects
}

#endif