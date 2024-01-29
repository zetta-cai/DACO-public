/*
 * PopularityAggregator: track aggregated uncached popularity for the objects locally uncached in some edge nodes (thread safe).
 *
 * NOTE: for memory efficiency of popularity metadata, we only track the global popular objects (with large max admission benefits) instead of all local uncached objects for selective popularity aggregation. For each global popular object, we only track (sum + top-k + bitmap) local uncached popularities instead of those of all edge nodes to reduce metadata overhead further.
 * 
 * By Siyuan Sheng (2023.09.03).
 */

#ifndef POPULARITY_AGGREGATOR_H
#define POPULARITY_AGGREGATOR_H

#include <map> // std::multimap
#include <string>
#include <unordered_map> // std::unordered_map

namespace covered
{
    // Forward declaration
    class PopularityAggregator;
}

#include "common/key.h"
#include "concurrency/rwlock.h"
#include "core/popularity/aggregated_uncached_popularity.h"
#include "core/popularity/collected_popularity.h"
#include "core/popularity/edgeset.h"
#include "core/popularity/fast_path_hint.h"
#include "core/popularity/preserved_edgeset.h"

namespace covered
{
    class PopularityAggregator
    {
    public:
        PopularityAggregator(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt);
        ~PopularityAggregator();

        bool getAggregatedUncachedPopularity(const Key& key, AggregatedUncachedPopularity& aggregated_uncached_popularity) const; // Return if key has aggregated uncached popularity
        uint32_t getTopkEdgecnt() const;
        bool isKeyBeingAdmitted(const Key& key) const;

        void updateAggregatedUncachedPopularity(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, FastPathHint& fast_path_hint); // Update aggregated uncached popularity for selective popularity aggregation
        void updatePreservedEdgesetForPlacement(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const Edgeset& placement_edgeset, const bool& is_global_cached); // Preserve edge nodes in placement edgeset for non-blocking placement deployment
        void clearPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx); // Clear preserved edge nodes for the given key at the source edge node for metadata releasing after local/remote admission notification (NOTE: is_global_cached MUST be true)

        uint64_t getSizeForCapacity() const;
    private:
        // NOTE: we MUST store Key in ordered list to locate lookup table during eviciton; use duplicate Keys in lookup table to update ordered list -> if we store Key pointer in ordered list and use duplicate popularity/LRU-order in lookup table, we still can locate lookup table during eviction, yet cannot locate the corresponding popularity entry / have to access all LRU entries to update ordered list
        typedef std::multimap<DeltaReward, AggregatedUncachedPopularity> benefit_popularity_multimap_t; // Aggregated popularities for each global popular objects sorted in ascending order of Delta rewards (i.e., max admission benefits)
        typedef benefit_popularity_multimap_t::iterator benefit_popularity_multimap_iter_t;
        typedef benefit_popularity_multimap_t::const_iterator benefit_popularity_multimap_const_iter_t;
        typedef std::unordered_map<Key, benefit_popularity_multimap_iter_t, KeyHasher> perkey_benefit_popularity_table_t; // Lookup table for benefit_popularity_multimap_t
        typedef perkey_benefit_popularity_table_t::iterator perkey_benefit_popularity_table_iter_t;
        typedef perkey_benefit_popularity_table_t::const_iterator perkey_benefit_popularity_table_const_iter_t;
        typedef std::unordered_map<Key, PreservedEdgeset, KeyHasher> perkey_preserved_edgeset_t; // Preserved edgeset for each key with non-blocking placement deployment

        // Utils

        bool getAggregatedUncachedPopularity_(const Key& key, AggregatedUncachedPopularity& aggregated_uncached_popularity) const; // Return if key has aggregated uncached popularity

        // Add a new aggregated uncached popularity for latest local uncached popularity
        void addAggregatedUncachedPopularityForNewKey_(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached);
        // If is_tracked_by_source_edge_node = true, add/update latest local uncached popularity for key in source edge node (maybe increase cache size usage)
        // Otherwise, remove old local uncached popularity for key in edge node if any (NEVER increase cache size usage)
        bool updateAggregatedUncachedPopularityForExistingKey_(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const uint32_t& source_edge_idx, const bool& is_tracked_by_source_edge_node, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached); // Return if add/remove local uncached popularity successfully

        // Update benefit-popularity multimap and per-key lookup table for new aggregated uncached popularity
        void addBenefitPopularityForNewKey_(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const AggregatedUncachedPopularity& new_aggregated_uncached_popularity, const bool& is_global_cached);
        // Update benefit-popularity multimap and per-key lookup table for existing yet updated aggregated uncached popularity
        void updateBenefitPopularityForExistingKey_(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const AggregatedUncachedPopularity& updated_aggregated_uncached_popularity, const bool& is_aggregated_uncached_popularity_empty, const bool& is_global_cached);

        // NOTE: we ONLY keep aggregated uncached popularities of selected objects with large max admission benefits for selective popularity aggregation
        void discardGlobalLessPopularObjects_();

        void checkPointers_() const;

        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;
        const uint32_t edgecnt_; // Come from CLI
        const uint64_t popularity_aggregation_capacity_bytes_; // Come from CLI
        const uint32_t topk_edgecnt_; // Come from CLI

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as PopularityAggregator may access aggregated uncached popularity of multiple keys within a single function (e.g., erase aggregated uncached popularities with relatively small max admission benefits)
        mutable Rwlock* rwlock_for_popularity_aggregator_;

        // Non-const shared variables
        uint64_t size_bytes_; // Cache size usage of popularity aggregator
        benefit_popularity_multimap_t benefit_popularity_multimap_; // NOTE: we use multimap instead of unordered_map to support duplicate Delta rewards (i.e., max admission benefits) for different global popular objects -> NOTE: if all local uncached popularities are removed during popularity aggregation (is_tracked = false) or after placement calculation (clear placement edgeset), else if capacity bytes for popularity aggregation are used up, we will erase the corresponding aggregated uncached popularity (unlike edge-level victim metadata in victim tracker that will never be erased due to cache margin bytes)
        perkey_benefit_popularity_table_t perkey_benefit_popularity_table_; // Track per-key iterator to lookup benefit_popularity_multimap_ for selective popularity aggregation (for unplaced keys)
        perkey_preserved_edgeset_t perkey_preserved_edgeset_; // Preserve edge nodes for each key being admitted for non-blocking placement deployment (for being-placed keys)
    };
}

#endif