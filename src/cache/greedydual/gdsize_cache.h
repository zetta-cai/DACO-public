/*
 * GDSizeCache: refer to lib/webcachesim/caches/gd_variants.*.
 *
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2023.11.21).
 */

#ifndef GDSIZE_CACHE_H
#define GDSIZE_CACHE_H

#include "cache/greedydual/greedy_dual_base.h"

namespace covered
{
    /*
      Greedy Dual Size policy
    */
    class GDSizeCache : public GreedyDualBase
    {
    public:
        GDSizeCache(const uint64_t& capacity_bytes);

        virtual ~GDSizeCache();
    protected:
        virtual long double ageValue_(const Key& key, const Value& value); // Calculate hval for cached/being-admited object
    private:
        static const std::string kClassName;
    };
}

#endif