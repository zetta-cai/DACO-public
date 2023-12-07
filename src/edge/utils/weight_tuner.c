#include "edge/utils/weight_tuner.h"

#include <sstream>

#include "common/util.h"

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

    // WeightTuner

    const double WeightTuner::EWMA_ALPHA = 0.1;
    const std::string WeightTuner::kClassName = "WeightTuner";

    WeightTuner::WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us) : remote_beacon_prob_(1.0 - 1.0 / static_cast<float>(edgecnt)), rwlock_for_weight_tuner_("rwlock_for_weight_tuner_")
    {
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        ewma_propagation_latency_clientedge_us_ = propagation_latency_clientedge_us;
        ewma_propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
        ewma_propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;

        updateWeightInfo_(); // Update weight_info_ for heuristic weight calculation

        // Dump initial weight info
        oss.clear();
        oss.str("");
        oss << "initial local hit weight: " << weight_info_.getLocalHitWeight() << ", cooperative hit weight: " << weight_info_.getCooperativeHitWeight() << ", remote beacon prob: " << remote_beacon_prob_;
        Util::dumpDebugMsg(instance_name_, oss.str());
    }

    WeightTuner::~WeightTuner() {}

    WeightInfo WeightTuner::getWeightInfo() const
    {
        // Acquire a read lock
        const std::string context_name = "WeightTuner::getWeightInfo()";
        rwlock_for_weight_tuner_.acquire_lock_shared(context_name);

        WeightInfo weight_info = weight_info_;

        rwlock_for_weight_tuner_.unlock_shared(context_name);
        return weight_info;
    }

    void WeightTuner::tuneWeightInfo(const uint32_t& cur_propagation_latency_clientedge_us, const uint32_t& cur_propagation_latency_crossedge_us, const uint32_t& cur_propagation_latency_edgecloud_us)
    {
        // Acquire a write lock
        const std::string context_name = "WeightTuner::tuneWeightInfo()";
        rwlock_for_weight_tuner_.acquire_lock(context_name);

        // Update latency by EWMA
        ewma_propagation_latency_clientedge_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_clientedge_us_ + EWMA_ALPHA * cur_propagation_latency_clientedge_us;
        ewma_propagation_latency_crossedge_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_crossedge_us_ + EWMA_ALPHA * cur_propagation_latency_crossedge_us;
        ewma_propagation_latency_edgecloud_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_edgecloud_us_ + EWMA_ALPHA * cur_propagation_latency_edgecloud_us;

        updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

        rwlock_for_weight_tuner_.unlock(context_name);
        return;
    }

    void WeightTuner::updateWeightInfo_()
    {
        // NOTE: NO need to acquire a write lock, which is NO need in constructor, or has been done in tuneWeightInfo()

        // Calculate latency of different accesses
        const Weight local_hit_latency = ewma_propagation_latency_clientedge_us_;
        const Weight cooperative_hit_latency = ewma_propagation_latency_clientedge_us_ + (remote_beacon_prob_ + 1) * ewma_propagation_latency_crossedge_us_;
        const Weight global_miss_latency = ewma_propagation_latency_clientedge_us_ + remote_beacon_prob_ * ewma_propagation_latency_crossedge_us_ + ewma_propagation_latency_edgecloud_us_;

        // Update weight info
        const Weight local_hit_weight = global_miss_latency - local_hit_latency; // w1
        const Weight cooperative_hit_weight = global_miss_latency - cooperative_hit_latency; // w2
        assert(local_hit_weight > cooperative_hit_weight && cooperative_hit_weight >= 0); // Weight verification
        weight_info_ = WeightInfo(local_hit_weight, cooperative_hit_weight);

        return;
    }
}