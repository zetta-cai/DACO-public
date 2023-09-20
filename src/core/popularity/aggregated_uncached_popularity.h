/*
 * AggregatedUncachedPopularity: aggregated uncached popularity including key, sum, top-k, and bitmap to reduce metadata overhead further under selective popularity aggregation.
 *
 * NOTE: as is_global_cached is continuously changed for the given key locally uncached in some edge node(s), we pass is_global_cached each time to calculate (max) admission benefit instead of tracking is_global_cached_ in AggregatedUncachedPopularity.
 * 
 * By Siyuan Sheng (2023.09.03).
 */

#ifndef AGGREGATED_UNCACHED_POPULARITY_H
#define AGGREGATED_UNCACHED_POPULARITY_H

#include <list>
#include <string>
#include <unordered_set>

#include "common/covered_common_header.h"
#include "common/key.h"

namespace covered
{
    class AggregatedUncachedPopularity
    {
    public:
        AggregatedUncachedPopularity();
        AggregatedUncachedPopularity(const Key& key, const uint32_t& edgecnt);
        ~AggregatedUncachedPopularity();

        const Key& getKey() const;
        ObjectSize getObjectSize() const;
        uint32_t getTopkListLength() const; // Get length k' of top-k list (k' <= topk_edgecnt)

        void update(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt, const ObjectSize& object_size);
        bool clear(const uint32_t& source_edge_idx, const uint32_t& topk_edgecnt); // Return if exist_edgecnt_ == 0 (i.e., NO local uncached popularity for key) after clear

        // For selective popularity aggregation
        DeltaReward calcMaxAdmissionBenefit(const bool& is_global_cached) const; // Max admission benefit if admit key into all top-k edge nodes

        // For trade-off-aware placement calculation
        DeltaReward calcAdmissionBenefit(const uint32_t& topicnt, const bool& is_global_cached, std::unordered_set<uint32_t>& placement_edgeset) const; // Admission benefit if admit key into top-i edge nodes (i <= top-k list length)

        uint64_t getSizeForCapacity() const;

        const AggregatedUncachedPopularity& operator=(const AggregatedUncachedPopularity& other);
    private:
        typedef std::pair<uint32_t, Popularity> edgeidx_popularity_pair_t;

        Popularity getLocalUncachedPopularityForExistingEdgeIdx_(const uint32_t& source_edge_idx) const;

        // For sum
        void minusLocalUncachedPopularityFromSum_(const Popularity& local_uncached_popularity);

        // For top-k list
        Popularity getTopiLocalUncachedPopularitySum_(const uint32_t& topicnt, std::unordered_set<uint32_t>& placement_edgeset) const; // Return sum of top-i uncached popularities (i <= top-k list length)
        std::list<edgeidx_popularity_pair_t>::const_iterator getTopkListIterForEdgeIdx_(const uint32_t& source_edge_idx) const; // Return an iterator pointing to edgeidx_popularity_pair_t if source_edge_idx is in top-k list
        bool updateTopkForExistingEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt); // Return if local uncached popularity is inserted into top-k list
        bool tryToInsertForNontopkEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt); // Return if local uncached popularity is inserted into top-k list
        void clearTopkForExistingEdgeIdx_(const uint32_t& source_edge_idx, const uint32_t& topk_edgecnt);

        static const std::string kClassName;

        Key key_;
        ObjectSize object_size_;
        Popularity sum_local_uncached_popularity_;
        std::list<edgeidx_popularity_pair_t> topk_edgeidx_local_uncached_popularity_pairs_; // Sorted by local uncached popularity in ascending order
        std::vector<bool> bitmap_;
        uint32_t exist_edgecnt_; // # of true in bitmap_
    };
}

#endif