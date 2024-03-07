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

    uint64_t WeightInfo::getSizeForCapacity() const
    {
        return sizeof(Weight) * 2;
    }
    
    const WeightInfo& WeightInfo::operator=(const WeightInfo& other)
    {
        local_hit_weight_ = other.local_hit_weight_;
        cooperative_hit_weight_ = other.cooperative_hit_weight_;
        return *this;
    }

    // WeightTuner

    const float WeightTuner::BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE = 100.0; // Tune prob every 100 beacon accesses for content discovery
    const double WeightTuner::EWMA_ALPHA = 0.1;
    const std::string WeightTuner::kClassName = "WeightTuner";

    WeightTuner::WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us) : rwlock_for_weight_tuner_("rwlock_for_weight_tuner_"), ewma_propagation_latency_clientedge_us_(propagation_latency_clientedge_us)
    {
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        local_beacon_access_cnt_ = 0.0;
        remote_beacon_access_cnt_ = 0.0;
        ewma_remote_beacon_prob_ = (1.0 - 1.0 / static_cast<float>(edgecnt));

        ewma_propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
        ewma_propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;

        updateWeightInfo_(); // Update weight_info_ for heuristic weight calculation

        // Dump initial weight info
        oss.clear();
        oss.str("");
        oss << "initial local hit weight: " << weight_info_.getLocalHitWeight() << ", cooperative hit weight: " << weight_info_.getCooperativeHitWeight() << ", remote beacon prob: " << ewma_remote_beacon_prob_;
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

    void WeightTuner::incrLocalBeaconAccessCnt()
    {
        #ifdef ENABLE_PROBABILITY_TUNING
        // Acquire a write lock
        const std::string context_name = "WeightTuner::incrLocalBeaconAccessCnt()";
        rwlock_for_weight_tuner_.acquire_lock(context_name);

        local_beacon_access_cnt_ += 1.0;

        // Update remote beacon probability for every tuning window
        if (local_beacon_access_cnt_ + remote_beacon_access_cnt_ >= BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE) // At the end of a tuning window
        {
            updateEwmaRemoteBeaconProb_();
        }

        updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

        rwlock_for_weight_tuner_.unlock(context_name);
        #endif

        return;
    }

    void WeightTuner::incrRemoteBeaconAccessCnt()
    {
        #ifdef ENABLE_PROBABILITY_TUNING
        // Acquire a write lock
        const std::string context_name = "WeightTuner::incrRemoteBeaconAccessCnt()";
        rwlock_for_weight_tuner_.acquire_lock(context_name);

        remote_beacon_access_cnt_ += 1.0;

        // Update remote beacon probability for every tuning window
        if (local_beacon_access_cnt_ + remote_beacon_access_cnt_ >= BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE) // At the end of a tuning window
        {
            updateEwmaRemoteBeaconProb_();
        }

        updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

        rwlock_for_weight_tuner_.unlock(context_name);
        #endif

        return;
    }

    void WeightTuner::updateEwmaCrossedgeLatency(const uint32_t& cur_propagation_latency_crossedge_us)
    {
        #ifdef ENABLE_WEIGHT_TUNING
        // Acquire a write lock
        const std::string context_name = "WeightTuner::updateEwmaCrossedgeLatency()";
        rwlock_for_weight_tuner_.acquire_lock(context_name);

        assert(cur_propagation_latency_crossedge_us > 0);

        // Update cross-edge latency by EWMA
        ewma_propagation_latency_crossedge_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_crossedge_us_ + EWMA_ALPHA * cur_propagation_latency_crossedge_us;

        updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

        rwlock_for_weight_tuner_.unlock(context_name);
        #endif

        return;
    }

    void WeightTuner::updateEwmaEdgecloudLatency(const uint32_t& cur_propagation_latency_edgecloud_us)
    {
        #ifdef ENABLE_WEIGHT_TUNING
        // Acquire a write lock
        const std::string context_name = "WeightTuner::updateEwmaEdgecloudLatency()";
        rwlock_for_weight_tuner_.acquire_lock(context_name);

        assert(cur_propagation_latency_edgecloud_us > 0);

        // Update edge-cloud latency by EWMA
        ewma_propagation_latency_edgecloud_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_edgecloud_us_ + EWMA_ALPHA * cur_propagation_latency_edgecloud_us;

        updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

        rwlock_for_weight_tuner_.unlock(context_name);
        #endif

        return;
    }

    // void WeightTuner::tuneWeightInfo()
    // {
    //     // Acquire a write lock
    //     const std::string context_name = "WeightTuner::tuneWeightInfo()";
    //     rwlock_for_weight_tuner_.acquire_lock(context_name);

    //     // Manually tune weight info
    //     updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning

    //     rwlock_for_weight_tuner_.unlock(context_name);
    //     return;
    // }

    uint64_t WeightTuner::getSizeForCapacity() const
    {
        // NOTE: NOT count sizeof(ewma_propagation_latency_clientedge_us_), which is actually NO need to be tracked and does NOT affect weight_info_ (we track it here just for better understanding of source code)
        return sizeof(float) + sizeof(uint32_t) * 2 + weight_info_.getSizeForCapacity();
    }

    void WeightTuner::updateEwmaRemoteBeaconProb_()
    {
        assert(local_beacon_access_cnt_ + remote_beacon_access_cnt_ >= BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE); // At the end of a tuning window

        // Calculate remote beacon prob of current window
        float tmp_remote_beacon_prob = remote_beacon_access_cnt_ / (local_beacon_access_cnt_ + remote_beacon_access_cnt_);
        assert(tmp_remote_beacon_prob >= 0 && tmp_remote_beacon_prob <= 1.0);

        // Update EWMA of remote beacon prob
        ewma_remote_beacon_prob_ = (1 - EWMA_ALPHA) * ewma_remote_beacon_prob_ + EWMA_ALPHA * tmp_remote_beacon_prob;

        // Clean for next window
        local_beacon_access_cnt_ = 0.0;
        remote_beacon_access_cnt_ = 0.0;

        return;
    }

    void WeightTuner::updateWeightInfo_()
    {
        // NOTE: NO need to acquire a write lock, which is NO need in constructor, or has been done in tuneWeightInfo()

        // Calculate latency of different accesses
        const Weight local_hit_latency = ewma_propagation_latency_clientedge_us_;
        const Weight cooperative_hit_latency = ewma_propagation_latency_clientedge_us_ + (ewma_remote_beacon_prob_ + 1) * ewma_propagation_latency_crossedge_us_;
        const Weight global_miss_latency = ewma_propagation_latency_clientedge_us_ + ewma_remote_beacon_prob_ * ewma_propagation_latency_crossedge_us_ + ewma_propagation_latency_edgecloud_us_;

        // Update weight info
        const Weight local_hit_weight = global_miss_latency - local_hit_latency; // w1
        const Weight cooperative_hit_weight = global_miss_latency - cooperative_hit_latency; // w2
        assert(local_hit_weight > cooperative_hit_weight && cooperative_hit_weight >= 0); // Weight verification
        weight_info_ = WeightInfo(local_hit_weight, cooperative_hit_weight);

        return;
    }
}