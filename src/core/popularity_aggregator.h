/*
 * PopularityAggregator: track aggregated uncached popularity for the objects locally uncached in some edge nodes (thread safe).
 *
 * NOTE: for memory efficiency of popularity metadata, we only track the global popular objects (with large max global admission benefits) instead of all local uncached objects for selective popularity aggregation. For each global popular object, we only track (sum + top-k + bitmap) local uncached popularities instead of those of all edge nodes to reduce metadata overhead further.
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
#include "core/popularity/aggregated_uncached_popularity.h"

namespace covered
{
    class PopularityAggregator
    {
    public:
        PopularityAggregator(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt);
        ~PopularityAggregator();

        void updateAggregatedUncachedPopularity(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached);
    private:
        // NOTE: we MUST store Key in ordered list to locate lookup table during eviciton; use duplicate Keys in lookup table to update ordered list -> if we store Key pointer in ordered list and use duplicate popularity/LRU-order in lookup table, we still can locate lookup table during eviction, yet cannot locate the corresponding popularity entry / have to access all LRU entries to update ordered list
        typedef std::multimap<DeltaReward, AggregatedUncachedPopularity> benefit_popularity_multimap_t; // Aggregated popularities for each global popular objects sorted in ascending order of Delta rewards (i.e., max global admission benefits)
        typedef benefit_popularity_multimap_t::iterator benefit_popularity_iter_t;
        typedef std::unordered_map<Key, benefit_popularity_iter_t, KeyHasher> perkey_benefit_popularity_table_t; // Lookup table for benefit_popularity_multimap_t
        typedef perkey_benefit_popularity_table_t::iterator perkey_benefit_popularity_iter_t;

        void addAggregatedUncachedPopularityForNewKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached);
        void updateAggregatedUncachedPopularityForExistingKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached);

        // NOTE: we ONLY keep aggregated uncached popularities of selected objects with large max global admission benefits for selective popularity aggregation
        void discardGlobalLessPopularObjects_();

        void checkPointers_() const;

        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;
        const uint32_t edgecnt_; // Come from CLI
        const uint64_t popularity_aggregation_capacity_bytes_; // Come from CLI
        const uint32_t topk_edgecnt_; // Come from CLI

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as PopularityAggregator may access aggregated uncached popularity of multiple keys within a single function (e.g., erase aggregated uncached popularities with relatively small max global admission benefits)
        mutable Rwlock* rwlock_for_popularity_aggregator_;

        // Non-const shared variables
        uint64_t size_bytes_; // Cache size usage of popularity aggregator
        benefit_popularity_multimap_t benefit_popularity_multimap_; // NOTE: we use multimap instead of unordered_map to support duplicate Delta rewards (i.e., max global admission benefits) for different global popular objects
        perkey_benefit_popularity_table_t perkey_benefit_popularity_table_;
    };
}

#endif