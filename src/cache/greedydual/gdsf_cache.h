/*
 * GDSFCache: refer to lib/webcachesim/caches/gd_variants.*.
 *
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2023.11.21).
 */

#ifndef GDSF_CACHE_H
#define GDSF_CACHE_H

#include "cache/greedydual/greedy_dual_base.h"

namespace covered
{
    /*
      Greedy Dual Size Frequency policy
    */
    class GDSFCache : public GreedyDualBase
    {
    public:
        GDSFCache(const uint64_t& capacity_bytes);
        virtual ~GDSFCache();

        virtual bool lookup(const Key& key, Value& value);
        virtual void admit(const Key& key, const Value& value);
        virtual void evict(const Key& key); // Evict the given key if any
        virtual void evict(Key& key, Value& value); // Evict the victim with the smallest hval
    protected:
        CacheStatsMapType _refsMap; // Frequency info of cached objects

        virtual long double ageValue_(const Key& key, const Value& value); // Calculate hval for cached/being-admited object
    private:
        static const std::string kClassName;
    };
}

#endif