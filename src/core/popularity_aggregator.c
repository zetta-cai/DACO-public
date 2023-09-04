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

    void PopularityAggregator::updateAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity)
    {
        checkPointers_();

        // Acquire a write lock to update aggregated popularity and max admission benefit atomically
        const std::string context_name = "PopularityAggregator::updateAggregatedPopularity()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        perkey_benefit_popularity_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        if (perkey_benefit_popularity_iter == perkey_benefit_popularity_table_.end()) // New key
        {
            addAggregatedPopularityForNewKey_(key, source_edge_idx, local_uncached_popularity);
        }
        else // Existing key
        {
            updateAggregatedPopularityForExistingKey_(key, source_edge_idx, local_uncached_popularity);
        }

        // TODO: Only keep aggregated popularities of objects with large max admission benefits for selective popularity aggregation

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    void PopularityAggregator::addAggregatedPopularityForNewKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedPopularity() for thread safety

        // Add new aggregated popularity for the new key
        AggregatedPopularity new_aggregated_popularity = AggregatedPopularity(key, edgecnt_);
        new_aggregated_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_);

        // TODO: END HERE

        // TODO: Update benefit_popularity_multimap_

        // TODO: Update perkey_benefit_popularity_table_

        return;
    }

    void updateAggregatedPopularityForExistingKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity);

    void PopularityAggregator::checkPointers_() const
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        return;
    }
}