/*
 * WeightTuner: manage cache-related weights for each edge node of COVERED (thread safe).
 *
 * NOTE: use heuristic weight calculation and latency-aware weight tuning.
 * 
 * By Siyuan Sheng (2023.12.05).
 */

#ifndef WEIGHT_TUNER_H
#define WEIGHT_TUNER_H

#define ENABLE_PROBABILITY_TUNING
#define ENABLE_WEIGHT_TUNING

#include <string>

#include "common/covered_common_header.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class WeightInfo
    {
    public:
        WeightInfo();
        WeightInfo(const Weight& local_hit_weight, const Weight& cooperative_hit_weight);
        ~WeightInfo();

        Weight getLocalHitWeight() const;
        Weight getCooperativeHitWeight() const;

        uint64_t getSizeForCapacity() const;

        const WeightInfo& operator=(const WeightInfo& other);
    private:
        static const std::string kClassName;

        Weight local_hit_weight_; // w1
        Weight cooperative_hit_weight_; // w2
    };

    class WeightTuner
    {
    public:
        static const float ENOUGH_BEACON_ACCESS_CNT_FOR_PROB_TUNING; // Threshold of total beacon access cnt to trigger remote beacon probability tuning
        static const double EWMA_ALPHA; // Alpha parameter for exponential weighted moving average of cross-edge/edge-cloud propagation latency

        WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us);
        ~WeightTuner();

        WeightInfo getWeightInfo() const;
        void incrLocalBeaconAccessCnt(); // NOTE: will invoke updateWeightInfo_ after increment local beacon access cnt
        void incrRemoteBeaconAccessCnt(); // NOTE: will invoke updateWeightInfo_ after increment remote beacon access cnt
        void updateEwmaCrossedgeLatency(const uint32_t& cur_propagation_latency_crossedge_us); // NOTE: will invoke updateWeightInfo_ after update cross-edge latency
        void updateEwmaEdgecloudLatency(const uint32_t& cur_propagation_latency_edgecloud_us); // NOTE: will invoke updateWeightInfo_ after update edge-cloud latency
        //void tuneWeightInfo();

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        void updateWeightInfo_();

        // Const variables
        std::string instance_name_;

        // For atomicity of non-const variables
        mutable Rwlock rwlock_for_weight_tuner_;

        // Non-const local/remote beacon access cnt and probability variables
        float local_beacon_access_cnt_; // Sender is beacon
        float remote_beacon_access_cnt_; // Sender is NOT beacon
        // NOTE: 0 means NOT consider remote content discovery overhead and 1 means always consider remote content discovery overhead; while (1.0 - 1.0/edgecnt) heuristically treat the probability of sender-is-beacon as 1.0/edgecnt under consistent-hashing-based DHT
        float remote_beacon_prob_; // Initialized by (1.0 - 1.0/edgecnt) from CLI

        // Non-const latency and weight variables (EWMA: Exponentially Weighted Moving Average)
        const uint32_t ewma_propagation_latency_clientedge_us_; // NOTE: client-edge latency does NOT affect weight_info_, as no matter local hit, cooperative hit, or cloud access have such latency
        uint32_t ewma_propagation_latency_crossedge_us_;
        uint32_t ewma_propagation_latency_edgecloud_us_;
        WeightInfo weight_info_;
    };
}

#endif