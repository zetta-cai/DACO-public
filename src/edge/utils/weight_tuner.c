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

    // Dump/load weight info for weight tuner snapshot
    void WeightInfo::dumpWeightInfo(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump local_hit_weight_
        fs_ptr->write((const char*)&local_hit_weight_, sizeof(Weight));

        // Dump cooperative_hit_weight_
        fs_ptr->write((const char*)&cooperative_hit_weight_, sizeof(Weight));

        return;
    }

    void WeightInfo::loadWeightInfo(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load local_hit_weight_
        fs_ptr->read((char*)&local_hit_weight_, sizeof(Weight));

        // Load cooperative_hit_weight_
        fs_ptr->read((char*)&cooperative_hit_weight_, sizeof(Weight));

        return;
    }

    // WeightTuner

    const float WeightTuner::BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE = 100.0; // Tune prob every 100 beacon accesses for content discovery
    const double WeightTuner::EWMA_ALPHA = 0.1;
    const std::string WeightTuner::kClassName = "WeightTuner";

    WeightTuner::WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us) : rwlock_for_weight_tuner_("rwlock_for_weight_tuner_"), ewma_propagation_latency_clientedge_us_(propagation_latency_clientedge_us), propagation_latency_edgecloud_us_(propagation_latency_edgecloud_us)
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
        oss << "initial cross-edge latency (us): " << ewma_propagation_latency_crossedge_us_ << ", edge-cloud latency (us): " << ewma_propagation_latency_edgecloud_us_ << ", local hit weight: " << weight_info_.getLocalHitWeight() << ", cooperative hit weight: " << weight_info_.getCooperativeHitWeight() << ", remote beacon prob: " << ewma_remote_beacon_prob_;
        Util::dumpNormalMsg(instance_name_, oss.str());
    }

    WeightTuner::~WeightTuner()
    {
        // Dump eventual weight info
        std::ostringstream oss;
        oss << "eventual cross-edge latency (us): " << ewma_propagation_latency_crossedge_us_ << ", edge-cloud latency (us): " << ewma_propagation_latency_edgecloud_us_ << ", local hit weight: " << weight_info_.getLocalHitWeight() << ", cooperative hit weight: " << weight_info_.getCooperativeHitWeight() << ", remote beacon prob: " << ewma_remote_beacon_prob_;
        Util::dumpNormalMsg(instance_name_, oss.str());
    }

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

        // Filter abnormal cross-edge latency which should < edge-cloud latency
        if (cur_propagation_latency_crossedge_us > propagation_latency_edgecloud_us_)
        {
            std::ostringstream oss;
            oss << "abnormal cross-edge latency: " << cur_propagation_latency_crossedge_us << " us, which should be less than edge-cloud latency: " << propagation_latency_edgecloud_us_ << " us (could be caused by CPU contention)";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }
        else
        {
            // Update cross-edge latency by EWMA
            ewma_propagation_latency_crossedge_us_ = (1 - EWMA_ALPHA) * ewma_propagation_latency_crossedge_us_ + EWMA_ALPHA * cur_propagation_latency_crossedge_us;
            assert(ewma_propagation_latency_crossedge_us_ >= 0);

            updateWeightInfo_(); // Update weight_info_ for latency-aware weight tuning
        }

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
        assert(ewma_propagation_latency_edgecloud_us_ >= 0);

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

    // Dump/load weight tuner snapshot

    void WeightTuner::dumpWeightTunerSnapshot(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump local_beacon_access_cnt_
        fs_ptr->write((const char*)&local_beacon_access_cnt_, sizeof(float));

        // Dump remote_beacon_access_cnt_
        fs_ptr->write((const char*)&remote_beacon_access_cnt_, sizeof(float));

        // Dump ewma_remote_beacon_prob_
        fs_ptr->write((const char*)&ewma_remote_beacon_prob_, sizeof(float));

        // NOTE: NO need to dump ewma_propagation_latency_clientedge_us_, which is not changed after initialization and not affect weights

        // Dump ewma_propagation_latency_crossedge_us_
        fs_ptr->write((const char*)&ewma_propagation_latency_crossedge_us_, sizeof(uint32_t));

        // Dump ewma_propagation_latency_edgecloud_us_
        fs_ptr->write((const char*)&ewma_propagation_latency_edgecloud_us_, sizeof(uint32_t));

        // Dump weight_info_
        weight_info_.dumpWeightInfo(fs_ptr);

        return;
    }

    void WeightTuner::loadWeightTunerSnapshot(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load local_beacon_access_cnt_
        fs_ptr->read((char*)&local_beacon_access_cnt_, sizeof(float));

        // Load remote_beacon_access_cnt_
        fs_ptr->read((char*)&remote_beacon_access_cnt_, sizeof(float));

        // Load ewma_remote_beacon_prob_
        fs_ptr->read((char*)&ewma_remote_beacon_prob_, sizeof(float));

        // NOTE: NO need to load ewma_propagation_latency_clientedge_us_, which is not changed after initialization and not affect weights

        // Load ewma_propagation_latency_crossedge_us_
        fs_ptr->read((char*)&ewma_propagation_latency_crossedge_us_, sizeof(uint32_t));

        // Load ewma_propagation_latency_edgecloud_us_
        fs_ptr->read((char*)&ewma_propagation_latency_edgecloud_us_, sizeof(uint32_t));

        // Load weight_info_
        weight_info_.loadWeightInfo(fs_ptr);

        return;
    }

    void WeightTuner::updateEwmaRemoteBeaconProb_()
    {
        assert(local_beacon_access_cnt_ + remote_beacon_access_cnt_ >= BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE); // At the end of a tuning window

        // Calculate remote beacon prob of current window
        float tmp_remote_beacon_prob = remote_beacon_access_cnt_ / (local_beacon_access_cnt_ + remote_beacon_access_cnt_);
        assert(tmp_remote_beacon_prob >= 0 && tmp_remote_beacon_prob <= 1.0);

        // Update EWMA of remote beacon prob
        ewma_remote_beacon_prob_ = (1 - EWMA_ALPHA) * ewma_remote_beacon_prob_ + EWMA_ALPHA * tmp_remote_beacon_prob;
        assert(ewma_remote_beacon_prob_ >= 0 && ewma_remote_beacon_prob_ <= 1.0);

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
        const Weight local_hit_weight = global_miss_latency - local_hit_latency; // w1 = ewma_remote_beacon_prob_ * ewma_propagation_latency_crossedge_us_ + ewma_propagation_latency_edgecloud_us_
        const Weight cooperative_hit_weight = global_miss_latency - cooperative_hit_latency; // w2 = ewma_propagation_latency_edgecloud_us_ - ewma_propagation_latency_crossedge_us_

        // Weight verification
        // assert(local_hit_weight >= 0);
        // assert(cooperative_hit_weight >= 0);
        // assert(local_hit_weight >= cooperative_hit_weight);
        if (local_hit_weight < 0 || cooperative_hit_weight < 0 || local_hit_weight < cooperative_hit_weight)
        {
            std::ostringstream oss;
            oss << "failed to verify weight info!" << std::endl;
            oss << "local hit weight: " << local_hit_weight << ", cooperative hit weight: " << cooperative_hit_weight << std::endl;
            oss << "local hit latency: " << local_hit_latency << ", cooperative hit latency: " << cooperative_hit_latency << ", global miss latency: " << global_miss_latency << std::endl;
            oss << "remote beacon prob: " << ewma_remote_beacon_prob_ << ", client-edge propagation latency: " << ewma_propagation_latency_clientedge_us_ << ", cross-edge propagation latency: " << ewma_propagation_latency_crossedge_us_ << ", edge-cloud propagation latency: " << ewma_propagation_latency_edgecloud_us_;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        
        weight_info_ = WeightInfo(local_hit_weight, cooperative_hit_weight);

        return;
    }
}