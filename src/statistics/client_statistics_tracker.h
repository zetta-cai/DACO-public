/*
 * ClientStatisticsTracker: track cur-slot/stable ClientRawStatistics for client workers, aggregate ClientRawStatistics into per-slot/stable ClientAggregatedStatistics, and dump ClientAggregatedStatistics (thread safe).
 *
 * NOTE: always update cur-slot client raw statistics for warmup phase (even if enalbing warmup speedup) and stresstest phase, while ONLY update stable client raw statistics for stresstest phase (i.e., is_stresstest_phase == false).
 *
 * NOTE: each client worker does NOT issue any request between warmup phase and stress test phase.
 *
 * NOTE: each client main thread (ClientWrapper) switches and updates cur-slot client raw statistics to finish warmup phase; simulator or evaluator in prototype monitors all client main threads to start stresstest phase.
 * 
 * By Siyuan Sheng (2023.05.21).
 */

#ifndef CLIENT_STATISTICS_TRACKER_H
#define CLIENT_STATISTICS_TRACKER_H

#include <atomic>
#include <string>

#include "statistics/client_raw_statistics.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    class ClientStatisticsTracker
    {
    public:
        static const uint32_t CURSLOT_CLIENT_RAW_STATISTICS_CNT;

        ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx, const uint32_t& curslot_client_raw_statistics_cnt = CURSLOT_CLIENT_RAW_STATISTICS_CNT);
        ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx);
        ~ClientStatisticsTracker();

        // (1) Update cur-slot/stable client raw statistics (invoked by client workers)

        // Update cur-slot/stable client raw statistics for cache hit ratio
        void updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest);
        void updateCooperativeHitcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest);
        void updateReqcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest);

        // Update cur-slot/stable client raw statistics for latency
        void updateLatency(const uint32_t& local_client_worker_idx, const uint32_t& latency_us, const bool& is_stresstest);

        // Update cur-slot/stable client raw statistics for read-write ratio
        void updateReadcnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest);
        void updateWritecnt(const uint32_t& local_client_worker_idx, const bool& is_stresstest);

        // (2) Switch cur-slot client raw statistics (invoked by client thread ClientWrapper)

        void switchCurslotForClientRawStatistics();
        bool isPerSlotAggregatedStatisticsStable(double& cache_hit_ratio); // Cache hit ratio is stable if cur slot <= prev slot

        // (3) Aggregate cur-slot/stable client raw statistics, and dump per-slot/stable client aggregated statistics for TotalStatisticsTracker (invoked by main client thread ClientWrapper)

        uint32_t aggregateAndDump(const std::string& filepath);

        // (4) Get client aggregated statistics

        std::vector<ClientAggregatedStatistics> getPerslotClientAggregatedStatistics() const;
        ClientAggregatedStatistics getStableClientAggregatedStatistics() const;
    private:
        static const std::string kClassName;

        // For cur-slot client raw statistics
        ClientRawStatistics* getCurslotClientRawStatisticsPtr_(const uint32_t& slot_idx) const;
        void curslotSwitchBarrier_() const; // Wait for all client workers for cur-slot time slot switching

        // Other utility functions
        void checkPointers_() const;

        // Used by aggregateAndDump() to check filepath and dump per-client statistics
        std::string checkFilepathForDump_(const std::string& filepath) const;

        // Load per-client statistics for TotalStatisticsTracker
        uint32_t load_(const std::string& filepath);

        // (A) Const shared variables

        // ClientStatisticsWrapper only uses client index to specify instance_name_ -> no need to maintain client_idx_
        std::string instance_name_;

        const bool allow_update_; // NOT allow statistics update when aggregation (by statistics_aggregator)
        const uint32_t perclient_workercnt_; // To track per-client-worker update status

        // (B) Non-const individual variables
        // NOTE: we have to use dynamic array for std::atomic<uint32_t>, as it does NOT have copy constructor and operator= for std::vector (e.g., resize() and push_back())

        // (B.1) For cur-slot client raw statistics
        // Accessed by both client workers and client wrapper
        std::atomic<uint32_t> cur_slot_idx_; // Index of current time slot
        // Track the update flag and status of each client worker for slot switch barrier
        std::atomic<bool>* perclientworker_curslot_update_flags_;
        std::atomic<uint64_t>* perclientworker_curslot_update_statuses_;
        std::vector<ClientRawStatistics*> curslot_client_raw_statistics_ptr_list_; // Track ClientRawStatistics of curslot_client_raw_statistics_cnt recent slots for per-slot aggregated statistics

        // (B.2) For stable client raw statistics
        // Accessed by both client workers and client wrapper
        ClientRawStatistics* stable_client_raw_statistics_ptr_;

        // (C) For per-slot client aggregated statistics
        // ONLY accessed by client wrapper
        std::vector<ClientAggregatedStatistics> perslot_client_aggregated_statistics_list_;

        // (D) For stable client aggregated statistics
        ClientAggregatedStatistics stable_client_aggregated_statistics_;
    };
}

#endif