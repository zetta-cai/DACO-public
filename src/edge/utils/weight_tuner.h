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

#include <fstream>
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
        WeightInfo(const Weight& local_hit_weight, const Weight& cooperative_hit_weight, const std::vector<Weight> local_hit_weights ,const std::vector<Weight> cooperative_hit_weights);
        ~WeightInfo();

        Weight getLocalHitWeight() const;
        Weight getCooperativeHitWeight() const;
        
        std::vector<Weight> getLocalHitWeights() const;
        std::vector<Weight> getCooperativeHitWeights() const;

        uint64_t getSizeForCapacity() const;

        const WeightInfo& operator=(const WeightInfo& other);

        // Dump/load weight info for weight tuner snapshot
        void dumpWeightInfo(std::fstream* fs_ptr) const;
        void loadWeightInfo(std::fstream* fs_ptr);
    private:
        static const std::string kClassName;

        Weight local_hit_weight_; // w1
        Weight cooperative_hit_weight_; // w2
        std::vector<Weight> local_hit_weights_;
        std::vector<Weight> cooperative_hit_weights_; // used for p2p: delta_local_i, [delta_remote_j]
    };

    class WeightTuner
    {
    public:
        static const float BEACON_ACCESS_CNT_FOR_PROB_WINDOW_SIZE; // Window size of beacon access cnt for remote beacon probability tuning
        static const double EWMA_ALPHA; // Alpha parameter for exponential weighted moving average of cross-edge/edge-cloud propagation latency

        WeightTuner(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const std::vector<uint32_t> p2p_latency_array = std::vector<uint32_t>());
        ~WeightTuner();

        WeightInfo getWeightInfo() const;
        // WeightInfo getWeightInfoP2P(const uint32_t& edge_idx) const;
        // std::vector<WeightInfo> getWeightInfoArray() const;
        void incrLocalBeaconAccessCnt(); // NOTE: will invoke updateEwmaRemoteBeaconProb_ and updateWeightInfo_ after increment local beacon access cnt
        void incrRemoteBeaconAccessCnt(); // NOTE: will invoke updateEwmaRemoteBeaconProb_ and updateWeightInfo_ after increment remote beacon access cnt
        void incrRemoteBeaconAccessCntArray(int j); // NOTE: will invoke updateEwmaRemoteBeaconProb_ and updateWeightInfo_ after increment remote beacon access cnt
        
        void updateEwmaCrossedgeLatency(const uint32_t& cur_propagation_latency_crossedge_us); // NOTE: will invoke updateWeightInfo_ after update cross-edge latency
        void updateEwmaEdgecloudLatency(const uint32_t& cur_propagation_latency_edgecloud_us); // NOTE: will invoke updateWeightInfo_ after update edge-cloud latency
        void updateEwmaCrossedgeLatency_of_j(int j, const uint32_t& cur_latency);
        uint32_t getEwmaCrossedgeLatency_of_j(int j);
        //void tuneWeightInfo();

        uint64_t getSizeForCapacity() const;

        // Dump/load weight tuner snapshot
        void dumpWeightTunerSnapshot(std::fstream* fs_ptr) const;
        void loadWeightTunerSnapshot(std::fstream* fs_ptr);

        bool getIsP2PEnable();
    private:
        static const std::string kClassName;

        void updateEwmaRemoteBeaconProb_();
        // void updateEwmaRemoteBeaconProbArray_();
        void updateWeightInfo_();
        // void updateWeightInfoP2P_();

        // Const variables
        std::string instance_name_;

        // For atomicity of non-const variables
        mutable Rwlock rwlock_for_weight_tuner_;

        // Non-const local/remote beacon access cnt and probability variables
        float local_beacon_access_cnt_; // Sender is beacon
        float remote_beacon_access_cnt_; // Sender is NOT beacon
        std::vector<float> remote_beacon_access_cnt_array_;
        // NOTE: 0 means NOT consider remote content discovery overhead and 1 means always consider remote content discovery overhead; while (1.0 - 1.0/edgecnt) heuristically treat the probability of sender-is-beacon as 1.0/edgecnt under consistent-hashing-based DHT
        float ewma_remote_beacon_prob_; // Initialized by (1.0 - 1.0/edgecnt) from CLI
        // std::vector<float> ewma_remote_beacon_prob_array_;
        // Non-const latency and weight variables (EWMA: Exponentially Weighted Moving Average)
        const uint32_t ewma_propagation_latency_clientedge_us_; // NOTE: client-edge latency does NOT affect weight_info_, as no matter local hit, cooperative hit, or cloud access have such latency
        const uint32_t propagation_latency_edgecloud_us_; // Used to filter abnormal cross-edge latency
        uint32_t ewma_propagation_latency_crossedge_us_;
        uint32_t ewma_propagation_latency_edgecloud_us_;

        std::vector<uint32_t> ewma_propagation_latency_crossedge_us_array_;
        WeightInfo weight_info_;
        // std::vector<WeightInfo> weight_info_array_; 

        bool is_p2p_enable;
    };
}

#endif