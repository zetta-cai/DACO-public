/*
 * ClientStatisticsTracker: track cur-slot/stable ClientRawStatistics for client workers, aggregate ClientRawStatistics into per-slot/stable ClientAggregatedStatistics to answer evaluator (thread safe).
 *
 * NOTE: always update cur-slot client raw statistics for warmup phase (even if enalbing warmup speedup) and stresstest phase, while ONLY update stable client raw statistics for stresstest phase (i.e., is_stresstest_phase == false).
 * 
 * By Siyuan Sheng (2023.05.21).
 */

#ifndef CLIENT_STATISTICS_TRACKER_H
#define CLIENT_STATISTICS_TRACKER_H

#include <atomic>
#include <string>

#include "common/bandwidth_usage.h"
#include "message/hitflag.h"
#include "statistics/client_raw_statistics.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    class ClientStatisticsTracker
    {
    public:
        static const uint32_t CURSLOT_CLIENT_RAW_STATISTICS_CNT;

        ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx, const uint32_t& curslot_client_raw_statistics_cnt = CURSLOT_CLIENT_RAW_STATISTICS_CNT);
        ~ClientStatisticsTracker();

        // (0) Get cur-slot/stable client raw statistics for debug (invoked by client workers)

        uint32_t getCurslotIdx() const;
        uint32_t getCurslotReqcnt(const uint32_t& local_client_worker_idx) const;
        // uint32_t getCurslotMaxLatency(const uint32_t& local_client_worker_idx) const; // NOTE: this is time-consuming

        // (1) Update cur-slot/stable client raw statistics (invoked by client workers)

        // Update cur-slot/stable client raw statistics for cache object hit ratio
        void updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase);
        void updateCooperativeHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase);
        void updateReqcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for cache byte hit ratio
        void updateLocalHitbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase);
        void updateCooperativeHitbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase);
        void updateReqbytes(const uint32_t& local_client_worker_idx, const uint32_t& object_size, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for latency
        void updateLatency(const uint32_t& local_client_worker_idx, const uint32_t& latency_us, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for read-write ratio
        void updateReadcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase);
        void updateWritecnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for cache utilization
        void updateCacheUtilization(const uint32_t& local_client_worker_idx, const uint64_t& closest_edge_cache_size_bytes, const uint64_t& closest_edge_cache_capacity_bytes, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for value size
        void updateWorkloadKeyValueSize(const uint32_t& local_client_worker_idx, const uint32_t& key_size, const uint32_t& value_size, const Hitflag& hitflag, const bool& is_stresstest_phase);

        // Update cur-slot/stable client raw statistics for bandwidth usage
        void updateBandwidthUsage(const uint32_t& local_client_worker_idx, const BandwidthUsage& bandwidth_usage, const bool& is_stresstest_phase);

        // (2) Switch cur-slot client raw statistics (invoked by client thread ClientWrapper)

        // TODO: if cur-slot client aggregated statistics is not precise enough, we can return cur-slot client raw statistics to evaluator for fine-grained statistics tracking
        ClientAggregatedStatistics switchCurslotForClientRawStatistics(const uint32_t& target_slot_idx);

        // (3) Aggregate cur-slot/stable client raw statistics when benchmark is finished (invoked by main client thread ClientWrapper)

        // Return last slot idx with two ClientAggregatedStatistics for last-slot and stable
        uint32_t aggregateForFinishrun(ClientAggregatedStatistics& lastslot_client_aggregated_statistics, ClientAggregatedStatistics& stable_client_aggregated_statistics);
    private:
        static const std::string kClassName;

        // For cur-slot client raw statistics
        ClientRawStatistics* getCurslotClientRawStatisticsPtr_(const uint32_t& slot_idx) const;
        void curslotSwitchBarrier_() const; // Wait for all client workers for cur-slot time slot switching

        // Other utility functions
        void checkPointers_() const;

        // (A) Const shared variables

        // ClientStatisticsWrapper only uses client index to specify instance_name_ -> no need to maintain client_idx_
        std::string instance_name_;

        const uint32_t perclient_workercnt_; // To track per-client-worker update status

        // (B) Non-const individual variables
        // NOTE: we have to use dynamic array for std::atomic<uint32_t>, as it does NOT have copy constructor and operator= for std::vector (e.g., resize() and push_back())

        // (B.1) For cur-slot client raw statistics
        // Accessed by both client workers and client wrapper
        std::atomic<uint32_t> cur_slot_idx_; // Index of current time slot
        // Track the update flag and status of each client worker for slot switch barrier
        std::atomic<bool>* perclientworker_curslot_update_flags_;
        std::atomic<uint64_t>* perclientworker_curslot_update_statuses_;
        std::vector<ClientRawStatistics*> curslot_client_raw_statistics_ptr_list_; // Track ClientRawStatistics of curslot_client_raw_statistics_cnt recent slots for per-slot total aggregated statistics in evaluator

        // (B.2) For stable client raw statistics
        // Accessed by both client workers and client wrapper
        ClientRawStatistics* stable_client_raw_statistics_ptr_;
    };
}

#endif