#include "common/covered_weight.h"

namespace covered
{
    // WeightInfo

    const std::string WeightInfo::kClassName = "WeightInfo";

    WeightInfo::WeightInfo()
    {
        local_hit_weight_ = 0;
        cooperative_hit_weight_ = 0;
    }

    WeightInfo::WeightInfo(const Weight& local_hit_weight, const Weight& cooperative_hit_weight)
    {
        local_hit_weight_ = local_hit_weight;
        cooperative_hit_weight_ = cooperative_hit_weight;
    }

    WeightInfo::~WeightInfo() {}

    Weight WeightInfo::getLocalHitWeight() const
    {
        return local_hit_weight_;
    }

    Weight WeightInfo::getCooperativeHitWeight() const
    {
        return cooperative_hit_weight_;
    }
    
    const WeightInfo& WeightInfo::operator=(const WeightInfo& other)
    {
        local_hit_weight_ = other.local_hit_weight_;
        cooperative_hit_weight_ = other.cooperative_hit_weight_;
        return *this;
    }

    // CoveredWeight

    const std::string CoveredWeight::kClassName = "CoveredWeight";

    Rwlock CoveredWeight::rwlock_for_covered_weight_("rwlock_for_covered_weight_");
    bool CoveredWeight::is_valid_ = false;
    WeightInfo CoveredWeight::weight_info_;

    WeightInfo CoveredWeight::getWeightInfo()
    {
        assert(is_valid_ == true);

        // Acquire a read lock
        const std::string context_name = "CoveredWeight::getWeightInfo()";
        rwlock_for_covered_weight_.acquire_lock_shared(context_name);

        WeightInfo weight_info = weight_info_;

        rwlock_for_covered_weight_.unlock_shared(context_name);
        return weight_info;
    }

    void CoveredWeight::setWeightInfo(const WeightInfo& weight_info)
    {
        // Acquire a write lock
        const std::string context_name = "CoveredWeight::setWeightInfo()";
        rwlock_for_covered_weight_.acquire_lock(context_name);

        weight_info_ = weight_info;
        is_valid_ = true;

        rwlock_for_covered_weight_.unlock(context_name);
        return;
    }
}