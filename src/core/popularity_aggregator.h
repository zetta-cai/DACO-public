/*
 * PopularityAggregator: track aggregated popularity for the objects locally uncached in some edge nodes (thread safe).
 *
 * NOTE: for memory efficiency of popularity metadata, we only track the global popular objects (with large max admission benefits) instead of all local uncached objects. For each global popular object, we only track (sum + top-k + bitmap) local uncached popularities instead of those of all edge nodes.
 * 
 * By Siyuan Sheng (2023.09.03).
 */

#ifndef POPULARITY_AGGREGATOR_H
#define POPULARITY_AGGREGATOR_H

#include <map> // std::multimap
#include <string>
#include <unordered_map> // std::unordered_map

#include "common/key.h"
#include "concurrency/rwlock.h"
#include "core/popularity/aggregated_popularity.h"

namespace covered
{
    class PopularityAggregator
    {
    public:
        PopularityAggregator(const uint32_t& edge_idx, const uint64_t& popularity_aggregation_capacity_bytes);
        ~PopularityAggregator();
    private:
        // NOTE: we MUST store Key in ordered list to locate lookup table during eviciton; use duplicate Keys in lookup table to update ordered list -> if we store Key pointer in ordered list and use duplicate popularity/LRU-order in lookup table, we still can locate lookup table during eviction, yet cannot locate the corresponding popularity entry / have to access all LRU entries to update ordered list
        typedef std::multimap<DeltaReward, AggregatedPopularity> benefit_popularity_multimap_t; // Aggregated popularities for each global popular objects sorted in ascending order of Delta rewards (i.e., max global admission benefits)
        typedef benefit_popularity_multimap_t::iterator benefit_popularity_iter_t;
        typedef std::unordered_map<Key, benefit_popularity_iter_t, KeyHasher> perkey_benefit_popularity_table_t; // Lookup table for benefit_popularity_multimap_t

        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;
        const uint64_t popularity_aggregation_capacity_bytes_;

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as PopularityAggregator may access aggregated uncached popularity of multiple keys within a single function (e.g., erase aggregated uncached popularities with relatively small max admission benefits)
        mutable Rwlock* rwlock_for_popularity_aggregator_;
    };
}

#endif