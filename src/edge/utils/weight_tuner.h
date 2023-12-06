/*
 * WeightTuner: manage cache-related weights for each edge node of COVERED (thread safe).
 *
 * NOTE: use heuristic weight calculation and latency-aware weight tuning.
 * 
 * By Siyuan Sheng (2023.12.05).
 */

#ifndef WEIGHT_TUNER_H
#define WEIGHT_TUNER_H

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

        const WeightInfo& operator=(const WeightInfo& other);
    private:
        static const std::string kClassName;

        Weight local_hit_weight_; // w1
        Weight cooperative_hit_weight_; // w2
    };

    class WeightTuner
    {
    public:
        static const double EWMA_ALPHA;

        WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us);
        ~WeightTuner();

        WeightInfo getWeightInfo() const;
        void tuneWeightInfo(const uint32_t& cur_propagation_latency_clientedge_us, const uint32_t& cur_propagation_latency_crossedge_us, const uint32_t& cur_propagation_latency_edgecloud_us);
    private:
        static const std::string kClassName;

        void updateWeightInfo_();

        // Const variables
        std::string instance_name_;
        const float remote_beacon_prob_; // Calculated by (1.0 - 1.0/edgecnt) from CLI (NOTE: 0 means NOT consider remote content discovery overhead and 1 means always consider remote content discovery overhead; while (1.0 - 1.0/edgecnt) heuristically treat the probability of sender-is-beacon as 1.0/edgecnt under consistent-hashing-based DHT) -> TODO: maybe collect request distribution to tune probability under real workloads (i.e., workload-aware probability tuning)

        // For atomicity of non-const variables
        mutable Rwlock rwlock_for_weight_tuner_;

        // Non-const variables (EWMA: Exponentially Weighted Moving Average)
        uint32_t ewma_propagation_latency_clientedge_us_;
        uint32_t ewma_propagation_latency_crossedge_us_;
        uint32_t ewma_propagation_latency_edgecloud_us_;
        WeightInfo weight_info_;
    };
}

#endif