#include "core/popularity_aggregator.h"

namespace covered
{
    const std::string PopularityAggregator::kClassName("PopularityAggregator");

    PopularityAggregator::PopularityAggregator(const uint32_t& edge_idx, const uint64_t& popularity_aggregation_capacity_bytes) : popularity_aggregation_capacity_bytes_(popularity_aggregation_capacity_bytes)
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
}