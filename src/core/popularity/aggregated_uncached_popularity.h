/*
 * AggregatedUncachedPopularity: aggregated uncached popularity including key, sum, top-k, and bitmap to reduce metadata overhead further under selective popularity aggregation.
 * 
 * By Siyuan Sheng (2023.09.03).
 */

#ifndef AGGREGATED_UNCACHED_POPULARITY_H
#define AGGREGATED_UNCACHED_POPULARITY_H

#include <list>
#include <string>

#include "cache/covered/common_header.h"
#include "common/key.h"

namespace covered
{
    typedef float Reward; // Local reward for local cached objects
    typedef float DeltaReward; // Local admission benefit for local uncached metadata, max global admission benefit for aggregated uncached popularity, global admission benefit and eviction cost for trade-off-aware cache placement and eviction

    class AggregatedUncachedPopularity
    {
    public:
        AggregatedUncachedPopularity();
        AggregatedUncachedPopularity(const Key& key, const uint32_t& edgecnt);
        ~AggregatedUncachedPopularity();

        void update(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt);

        DeltaReward getMaxGlobalAdmissionBenefit(const bool& is_cooperative_cached) const;

        uint64_t getSizeForCapacity() const;

        const AggregatedUncachedPopularity& operator=(const AggregatedUncachedPopularity& other);
    private:
        typedef std::pair<uint32_t, Popularity> edgeidx_popularity_pair_t;

        Popularity getLocalUncachedPopularityForExistingEdgeIdx_(const uint32_t& source_edge_idx) const;

        // For sum
        void minusLocalUncachedPopularityFromSum_(const Popularity& local_uncached_popularity);

        // For top-k list
        std::list<edgeidx_popularity_pair_t>::const_iterator getTopkListIterForEdgeIdx_(const uint32_t& source_edge_idx) const;
        bool tryToInsertForNontopkEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt); // Return if local uncached popularity is inserted into top-k list
        bool updateTopkForExistingEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt); // Return if local uncached popularity is inserted into top-k list

        static const std::string kClassName;

        Key key_;
        Popularity sum_local_uncached_popularity_;
        std::list<edgeidx_popularity_pair_t> topk_edgeidx_local_uncached_popularity_pairs_; // Sorted by local uncached popularity in ascending order
        std::vector<bool> bitmap_;
    };
}

#endif