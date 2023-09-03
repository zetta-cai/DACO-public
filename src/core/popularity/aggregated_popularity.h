/*
 * AggregatedPopularity: aggregated uncached popularity including key, sum, top-k, and bitmap for selective popularity aggregation with limited metadata overhead.
 * 
 * By Siyuan Sheng (2023.09.03).
 */

#ifndef AGGREGATED_POPULARITY_H
#define AGGREGATED_POPULARITY_H

#include <string>

#include "cache/covered/common_header.h"
#include "common/key.h"

namespace covered
{
    typedef float Reward; // Local reward for local cached objects
    typedef float DeltaReward; // Local admission benefit for local uncached objects, global admission benefit for aggregated uncached popularity, global admission benefit and eviction cost for trade-off-aware cache placement and eviction

    class AggregatedPopularity
    {
    public:
        AggregatedPopularity(const uint32_t& edgecnt);
        ~AggregatedPopularity();

        DeltaReward getMaxAdmissionBenefit(const bool& is_cooperative_cached) const;
    private:
        static const std::string kClassName;

        Key key_;
        Popularity sum_local_uncached_popularity_;
        std::list<Popularity> topk_local_uncached_popularity_;
        std::vector<bool> bitmap_;
    };
}

#endif