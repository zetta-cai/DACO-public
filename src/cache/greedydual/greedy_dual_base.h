/*
 * GreedyDualBase: refer to lib/webcachesim/caches/gd_variants.*.
 *
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2023.11.21).
 */

#ifndef GREEDY_DUAL_BASE_H
#define GREEDY_DUAL_BASE_H

#include <map>
#include <queue>
#include <string>
#include <unordered_map>

//#include "cache.h"
//#include "cache_object.h"

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    /*
      GD: greedy dual eviction (base class)

      [implementation via heap: O(log n) time for each cache miss]
    */
    class GreedyDualBase
    {
    public:
        GreedyDualBase(const uint64_t& capacity_bytes);

        virtual ~GreedyDualBase();

        virtual bool exists(const Key& key); // Check if key exists in cache (NOT udpate cache metadata)

        virtual bool lookup(const Key& key, Value& value);
        virtual bool update(const Key& key, const Value& value);
        virtual void admit(const Key& key, const Value& value);
        virtual void evict(const Key& key); // Evict the given key if any
        virtual void evict(Key& key, Value& value); // Evict the victim with the smallest hval
    protected:
        typedef std::multimap<long double, std::pair<Key, Value>> ValueMapType;
        typedef ValueMapType::iterator ValueMapIteratorType;
        typedef std::unordered_map<Key, ValueMapIteratorType, KeyHasher> GdCacheMapType;
        typedef std::unordered_map<Key, uint64_t, KeyHasher> CacheStatsMapType;

        virtual long double ageValue_(const Key& key, const Value& value); // Calculate hval for cached/being-admited object
        virtual void hit_(const Key& key);

        // basic cache properties
        const uint64_t _cacheSize; // size of cache in bytes
        uint64_t _currentSize; // total size of objects in cache in bytes

        // The GD current value (aging factor L)
        long double _currentL;
        // Ordered multi map of GD values and key-value pairs
        ValueMapType _valueMap;
        // Lookup table to find objects via unordered_map
        GdCacheMapType _cacheMap;
    private:
        static const std::string kClassName;
    };
}

#endif