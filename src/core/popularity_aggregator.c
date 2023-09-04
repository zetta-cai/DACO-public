#include "core/popularity_aggregator.h"

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
    }
    
    PopularityAggregator::~PopularityAggregator()
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        delete rwlock_for_popularity_aggregator_;
        rwlock_for_popularity_aggregator_ = NULL;
    }

    void PopularityAggregator::updateAggregatedUncachedPopularity(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached)
    {
        checkPointers_();

        // Acquire a write lock to update aggregated uncached popularity and max global admission benefit atomically
        const std::string context_name = "PopularityAggregator::updateAggregatedUncachedPopularity()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        perkey_benefit_popularity_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        if (perkey_benefit_popularity_iter == perkey_benefit_popularity_table_.end()) // New key
        {
            addAggregatedUncachedPopularityForNewKey_(key, source_edge_idx, local_uncached_popularity, is_cooperative_cached);
        }
        else // Existing key
        {
            updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, local_uncached_popularity, is_cooperative_cached);
        }

        // NOTE: we try to discard global less populart objects even if we update aggregated uncached popularity for an existing key (besides add aggregated uncached popularity for a new key), as AggregatedUncachedPopularity could add new local uncached popularity for the existing key
        // NOTE: this is different from local uncached metadata, which will NOT increase cache size usage if update local uncached metadata for an existing key (ONLY trigger removal if add local uncached metadata for a new key)

        // Discard the objects with small max global admission benefits if popularity aggregation capacity bytes are used up
        discardGlobalLessPopularObjects_();

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    void PopularityAggregator::addAggregatedUncachedPopularityForNewKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        // Prepare new aggregated uncached popularity for the new key
        AggregatedUncachedPopularity new_aggregated_uncached_popularity(key, edgecnt_);
        new_aggregated_uncached_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_);

        // Prepare new max global admission benefit for the new key
        DeltaReward new_max_global_admission_benefit = new_aggregated_uncached_popularity.getMaxGlobalAdmissionBenefit(is_cooperative_cached);

        // Insert new aggregated uncached popularity and new max global admission benefit into benefit_popularity_multimap_ for the new key
        benefit_popularity_iter_t new_benefit_popularity_iter = benefit_popularity_multimap_.insert(std::pair(new_max_global_admission_benefit, new_aggregated_uncached_popularity));

        // Insert new key with new benfit_popularity_iter into perkey_benefit_popularity_table_
        perkey_benefit_popularity_table_.insert(std::pair(key, new_benefit_popularity_iter));

        return;
    }

    void updateAggregatedUncachedPopularityForExistingKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const bool& is_cooperative_cached);

    void PopularityAggregator::discardGlobalLessPopularObjects_()
    {
        while (true)
        {
            // TODO: (END HERE) maintain size_bytes_ for cache size usage of popularity aggregator

            // Check if popularity aggregation capacity bytes are used up
            if (size_bytes_ <= popularity_aggregation_capacity_bytes_)
            {
                break;
            }
            else
            {
                // TODO: Find the object with the minimum max global admission benefit
                
                // TODO: Remove it from benefit_popularity_multimap_ and perkey_benefit_popularity_table_
            }
        }
    }

    void PopularityAggregator::checkPointers_() const
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        return;
    }
}