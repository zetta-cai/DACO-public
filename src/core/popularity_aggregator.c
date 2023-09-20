#include "core/popularity_aggregator.h"

#include "common/util.h"

namespace covered
{
    const std::string PopularityAggregator::kClassName("PopularityAggregator");

    PopularityAggregator::PopularityAggregator(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt) : edgecnt_(edgecnt), popularity_aggregation_capacity_bytes_(popularity_aggregation_capacity_bytes), topk_edgecnt_(topk_edgecnt)
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_popularity_aggregator_";
        rwlock_for_popularity_aggregator_ = new Rwlock(oss.str());
        assert(rwlock_for_popularity_aggregator_ != NULL);

        size_bytes_ = 0;
        benefit_popularity_multimap_.clear();
        perkey_benefit_popularity_table_.clear();
    }
    
    PopularityAggregator::~PopularityAggregator()
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        delete rwlock_for_popularity_aggregator_;
        rwlock_for_popularity_aggregator_ = NULL;
    }

    bool PopularityAggregator::getAggregatedUncachedPopularity(const Key& key, AggregatedUncachedPopularity& aggregated_uncached_popularity) const
    {
        bool is_found = false;

        perkey_benefit_popularity_const_iter_t perkey_benefit_popularity_const_iter = perkey_benefit_popularity_table_.find(key);
        if (perkey_benefit_popularity_const_iter != perkey_benefit_popularity_table_.end())
        {
            benefit_popularity_iter_t benefit_popularity_iter = perkey_benefit_popularity_const_iter->second;
            assert(benefit_popularity_iter != benefit_popularity_multimap_.end());
            aggregated_uncached_popularity = benefit_popularity_iter->second; // Deep copy

            is_found = true;
        }

        return is_found;
    }

    uint32_t PopularityAggregator::getTopkEdgecnt() const
    {
        return topk_edgecnt_;
    }

    void PopularityAggregator::updateAggregatedUncachedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached)
    {
        checkPointers_();

        // Acquire a write lock to update aggregated uncached popularity and max admission benefit atomically
        const std::string context_name = "PopularityAggregator::updateAggregatedUncachedPopularity()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        perkey_benefit_popularity_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);

        const bool is_tracked_by_source_edge_node = collected_popularity.isTracked(); // If key is tracked by local uncached metadata in the source edge node (i.e., if local uncached popularity is valid)
        Popularity local_uncached_popularity = 0.0;
        ObjectSize object_size = 0;
        if (is_tracked_by_source_edge_node) // Add/update latest local uncached popularity for key in source edge node into new/existing aggregated uncached popularity
        {
            local_uncached_popularity = collected_popularity.getLocalUncachedPopularity();
            object_size = collected_popularity.getObjectSize();
            if (perkey_benefit_popularity_iter == perkey_benefit_popularity_table_.end()) // New key
            {
                addAggregatedUncachedPopularityForNewKey_(key, source_edge_idx, local_uncached_popularity, object_size, is_global_cached);
            }
            else // Existing key
            {
                // Add/update local uncached popularity of source edge node
                updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, object_size, is_global_cached);
            }

            // NOTE: we try to discard global less populart objects even if we update aggregated uncached popularity for an existing key (besides add aggregated uncached popularity for a new key), as AggregatedUncachedPopularity could add new local uncached popularity for the existing key
            // NOTE: this is different from local uncached metadata, which will NOT increase cache size usage if update local uncached metadata for an existing key (ONLY trigger removal if add local uncached metadata for a new key)

            // Discard the objects with small max admission benefits if popularity aggregation capacity bytes are used up
            discardGlobalLessPopularObjects_();
        }
        else // Remove old local uncached popularity for key in source edge node from existing aggregated uncached popularity if any
        {
            // NOTE: NO need to remove old local uncached popularity if without aggregated uncached popularity for the key

            if (perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end()) // Existing key
            {
                // Remove old local uncached popularity of source edge node if any
                // NOTE: local_uncached_popularity and object_size are NOT used when is_tracked_by_source_edge_node = false
                updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, object_size, is_global_cached);
            }

            // NOTE: NO need to try to discard global less popular objects here, as removing old local uncached popularity if any will NEVER increase cache size usage
        }

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    void PopularityAggregator::clearAggregatedUncachedPopularityAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        // Clear old local uncached popularity of source edge node if any, and update max admission benefit for the given key
        // NOTE: local_uncached_popularity and object_size are NOT used when is_tracked_by_source_edge_node = false
        updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, false, 0.0, 0, true);

        // TODO: After introducing non-blocking cache admission placement:
        // (i) old local uncached popularity should be cleared right after placement calculation -> assert NO old local uncached popularity exists for the given key;
        // (ii) we should clear preserved edge idx / bitmap for the source edge node after admission -> assert preserved edge idx / bitmap MUST exist (NOTE: popularity aggregation capacity bytes should NOT discard preserved edge idx / bitmap!!!)

        return;
    }

    void PopularityAggregator::addAggregatedUncachedPopularityForNewKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        // Prepare new aggregated uncached popularity for the new key
        AggregatedUncachedPopularity new_aggregated_uncached_popularity(key, edgecnt_);
        new_aggregated_uncached_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_, object_size);

        // Prepare new max admission benefit for the new key
        DeltaReward new_max_admission_benefit = new_aggregated_uncached_popularity.calcMaxAdmissionBenefit(is_global_cached);

        // Insert new aggregated uncached popularity and new max admission benefit into benefit_popularity_multimap_ for the new key
        benefit_popularity_iter_t new_benefit_popularity_iter = benefit_popularity_multimap_.insert(std::pair(new_max_admission_benefit, new_aggregated_uncached_popularity));
        size_bytes_ += sizeof(DeltaReward); // Max admission benefit
        size_bytes_ += new_aggregated_uncached_popularity.getSizeForCapacity(); // Aggrgated uncached popularity

        // Insert new key with new benfit_popularity_iter into perkey_benefit_popularity_table_
        perkey_benefit_popularity_table_.insert(std::pair(key, new_benefit_popularity_iter));
        size_bytes_ += key.getKeyLength(); // Key
        size_bytes_ += sizeof(benefit_popularity_iter_t); // Lookup table iterator

        return;
    }

    void PopularityAggregator::updateAggregatedUncachedPopularityForExistingKey_(const Key& key, const uint32_t& source_edge_idx, const bool& is_tracked_by_source_edge_node, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        // Find the aggregated uncached popularity for the existing key
        perkey_benefit_popularity_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        assert(perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end()); // For existing key
        benefit_popularity_iter_t benefit_popularity_iter = perkey_benefit_popularity_iter->second;
        assert(benefit_popularity_iter != benefit_popularity_multimap_.end());
        AggregatedUncachedPopularity existing_aggregated_uncached_popularity = benefit_popularity_iter->second; // Deep copy
        const uint64_t old_aggregated_uncached_popularity_size_bytes = existing_aggregated_uncached_popularity.getSizeForCapacity();

        // Update the aggregated uncached popularity for the existing key
        bool is_aggregated_uncached_popularity_empty = false;
        if (is_tracked_by_source_edge_node) // Add/update local uncached popularity of source edge node
        {
            existing_aggregated_uncached_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_, object_size);
        }
        else // Clear aggregated uncached popularity to remove old local uncached popularity of source edge node if any
        {
            is_aggregated_uncached_popularity_empty = existing_aggregated_uncached_popularity.clear(source_edge_idx, topk_edgecnt_);
        }
        const uint64_t new_aggregated_uncached_popularity_size_bytes = existing_aggregated_uncached_popularity.getSizeForCapacity();
        if (is_tracked_by_source_edge_node)
        {
            assert(new_aggregated_uncached_popularity_size_bytes >= old_aggregated_uncached_popularity_size_bytes); // Maybe increase cache size usage
        }
        else
        {
            assert(new_aggregated_uncached_popularity_size_bytes <= old_aggregated_uncached_popularity_size_bytes); // NEVER increase cache size usage
        }

        // Remove old benefit-popularity pair for the given key
        size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(DeltaReward)); // Old max admission benefit
        size_bytes_ = Util::uint64Minus(size_bytes_, old_aggregated_uncached_popularity_size_bytes); // Old aggregated uncached popularity
        benefit_popularity_multimap_.erase(benefit_popularity_iter);

        if (is_aggregated_uncached_popularity_empty) // NO need aggregated uncached popularity for the given key
        {
            perkey_benefit_popularity_table_.erase(perkey_benefit_popularity_iter);
            size_bytes_ = Util::uint64Minus(size_bytes_, key.getKeyLength()); // Erased key
            size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(benefit_popularity_iter_t)); // Erased lookup table iterator
        }
        else // Still need aggregated uncached popularity for the given key
        {
            // Calculate a new max admission benefit for the existing key
            DeltaReward new_max_admission_benefit = existing_aggregated_uncached_popularity.calcMaxAdmissionBenefit(is_global_cached);

            // Update benefit_popularity_multimap_ for the new max admission benefit of the existing key
            benefit_popularity_iter = benefit_popularity_multimap_.insert(std::pair(new_max_admission_benefit, existing_aggregated_uncached_popularity));
            size_bytes_ += sizeof(DeltaReward); // New max admission benefit
            size_bytes_ += new_aggregated_uncached_popularity_size_bytes; // New aggregated uncached popularity

            // Update perkey_benefit_popularity_table_ for the new benefit_popularity_iter of the existing key
            perkey_benefit_popularity_iter->second = benefit_popularity_iter;
        }

        return;
    }

    void PopularityAggregator::discardGlobalLessPopularObjects_()
    {
        while (true)
        {
            // Check if popularity aggregation capacity bytes are used up
            if (size_bytes_ <= popularity_aggregation_capacity_bytes_)
            {
                break;
            }
            else
            {
                // Find the object with the minimum max admission benefit
                benefit_popularity_iter_t min_benefit_popularity_iter = benefit_popularity_multimap_.begin();
                assert(min_benefit_popularity_iter != benefit_popularity_multimap_.end());
                AggregatedUncachedPopularity tmp_aggregated_uncached_popularity = min_benefit_popularity_iter->second; // Aggregated uncached popularity with minimum max admission benefit
                
                // Remove it from benefit_popularity_multimap_
                benefit_popularity_multimap_.erase(min_benefit_popularity_iter);
                size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(DeltaReward)); // Max admission benefit
                size_bytes_ = Util::uint64Minus(size_bytes_, tmp_aggregated_uncached_popularity.getSizeForCapacity()); // Aggrgated uncached popularity
                
                // Remove it from perkey_benefit_popularity_table_
                Key tmp_key = tmp_aggregated_uncached_popularity.getKey(); // Key with minimum max admission benefit
                perkey_benefit_popularity_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(tmp_key);
                assert(perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end());
                perkey_benefit_popularity_table_.erase(perkey_benefit_popularity_iter);
                size_bytes_ = Util::uint64Minus(size_bytes_, tmp_key.getKeyLength()); // Key
                size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(benefit_popularity_iter_t)); // Lookup table iterator
            }
        }

        return;
    }

    void PopularityAggregator::checkPointers_() const
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        return;
    }
}