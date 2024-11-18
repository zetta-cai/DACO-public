/*
 * LrukCache: refer to lib/webcachesim/caches/gd_variants.*.
 *
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2023.11.21).
 */

#ifndef LRUK_CACHE_H
#define LRUK_CACHE_H

#include "cache/greedydual/greedy_dual_base.h"

namespace covered
{
    /*
      LRU-K policy (Siyuan: a GreedyDual version instead of LRU-K in the original paper)
    */
    class LRUKCache : public GreedyDualBase
    {
    public:
        LRUKCache(const uint64_t& capacity_bytes);
        virtual ~LRUKCache();

        virtual bool lookup(const Key& key, Value& value);
        virtual bool update(const Key& key, const Value& value);
        virtual void admit(const Key& key, const Value& value);
        virtual bool evict(const Key& key, Value& value); // Evict the given key if any
        virtual void evict(Key& key, Value& value); // Evict the victim with the smallest hval
    protected:
        typedef std::unordered_map<Key, std::queue<uint64_t>, KeyHasher> LrukMapType;

        LrukMapType _reqsMap; // Request timepoints as cache access history of cached objects
        unsigned int _tk; // K of backward-k distance (2 by default)
        uint64_t _curTime;

        virtual long double ageValue_(const Key& key, const Value& value); // Calculate hval for cached/being-admited object
    private:
        static const std::string kClassName;
    };
}

#endif