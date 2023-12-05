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

        WeightTuner(const uint32_t& edge_idx, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us);
        ~WeightTuner();

        WeightInfo getWeightInfo() const;
        void tuneWeightInfo(const uint32_t& cur_propagation_latency_clientedge_us, const uint32_t& cur_propagation_latency_crossedge_us, const uint32_t& cur_propagation_latency_edgecloud_us);
    private:
        static const std::string kClassName;

        void updateWeightInfo_();

        // Const variables
        std::string instance_name_;

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