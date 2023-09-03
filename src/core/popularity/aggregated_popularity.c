#include "core/popularity/aggregated_popularity.h"

namespace covered
{
    const std::string AggregatedPopularity::kClassName("AggregatedPopularity");

    AggregatedPopularity::AggregatedPopularity(const uint32_t& edgecnt) : key_(), sum_local_uncached_popularity_(0.0)
    {
        topk_local_uncached_popularity_.clear();
        bitmap_.resize(edgecnt, false);
    }
    
    AggregatedPopularity::~AggregatedPopularity() {}

    DeltaReward AggregatedPopularity::getMaxAdmissionBenefit(const bool& is_cooperative_cached) const
    {
        // TODO: Use a heuristic or learning-based approach for parameter tuning to calculate delta rewards for max admission benefits (refer to state-of-the-art studies such as LRB and GL-Cache)

        DeltaReward max_admission_benefit = 0.0;
        if (is_cooperative_cached) // Redirected cache hits become local cache hits for the edge nodes with top-k local uncached popularity
        {
            // max_admission_benefit = (w1 - w2) * topk_local_uncached_popularity_;
        }
        else // Global cache misses become local cache hits for the edge nodes with top-k local uncached popularity, and global cache misses become redirected cache hits for other edge nodes
        {
            // max_admission_benefit = w1 * topk_local_uncached_popularity_ + w2 * (sum_local_uncached_popularity_ - topk_local_uncached_popularity_);
        }
        return max_admission_benefit;
    }
}