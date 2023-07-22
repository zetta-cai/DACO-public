/*
 * ClientStatisticsTracker: track and dump per-client statistics (thread safe).
 *
 * NOTE: always update intermediate statistics for warmup phase (even if enalbing warmup speedup) and stresstest phase, while ONLY update stable statistics for stresstest phase (i.e., is_stresstest_phase == false).
 *
 * NOTE: each client worker does NOT issue any request between warmup phase and stress test phase.
 *
 * NOTE: each client main thread (ClientWrapper) switches intermediate statistics to finish warmup phase; simulator or evaluator in prototype monitors all client main threads to start stresstest phase.
 * 
 * By Siyuan Sheng (2023.05.21).
 */

#ifndef CLIENT_STATISTICS_TRACKER_H
#define CLIENT_STATISTICS_TRACKER_H

#include <atomic>
#include <string>

namespace covered
{
    class ClientStatisticsTracker
    {
    public:
        ClientStatisticsTracker(uint32_t perclient_workercnt, const uint32_t& client_idx);
        ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx);
        ~ClientStatisticsTracker();

        //uint32_t getPerclientWorkercnt() const;
        //uint32_t getLatencyHistogramSize() const;

        // (1) Update intermediate/stable statistics (invoked by client workers)

        // Update per-client hit ratio intermediate/stable statistics
        void updateLocalHitcnt(const uint32_t& local_client_worker_idx, const bool& is_warmup_phase);
        void updateCooperativeHitcnt(const uint32_t& local_client_worker_idx, const bool& is_warmup_phase);
        void updateReqcnt(const uint32_t& local_client_worker_idx, const bool& is_warmup_phase);

        // Update per-client latency intermediate/stable statistics
        void updateLatency(const uint32_t& latency_us, const bool& is_warmup_phase);

        // Update read-write intermediate/stable statistics
        void updateReadcnt(const uint32_t& local_client_worker_idx, const bool& is_warmup_phase);
        void updateWritecnt(const uint32_t& local_client_worker_idx, const bool& is_warmup_phase);

        // (2) Switch time slot for intermediate statistics (invoked by client thread ClientWrapper)

        // (3) Dump intermediate/stable statistics (invoked by main client thread ClientWrapper)

        // Dump per-client statistics for TotalStatisticsTracker
        uint32_t dump(const std::string& filepath) const;

        // Get per-client statistics for aggregation
        std::atomic<uint32_t>* getPerclientworkerLocalHitcnts() const;
        std::atomic<uint32_t>* getPerclientworkerCooperativeHitcnts() const;
        std::atomic<uint32_t>* getPerclientworkerReqcnts() const;
        std::atomic<uint32_t>* getLatencyHistogram() const;
        std::atomic<uint32_t>* getPerclientworkerReadcnts() const;
        std::atomic<uint32_t>* getPerclientworkerWritecnts() const;
    private:
        static const std::string kClassName;

        // Used by dump() to check filepath and dump per-client statistics
        std::string checkFilepathForDump_(const std::string& filepath) const;
        uint32_t dumpPerclientWorkercnt_(std::fstream* fs_ptr) const;
        uint32_t dumpPerclientworkerLocalHitcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpPerclientworkerCooperativeHitcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpPerclientworkerReqcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpLatencyHistogramSize_(std::fstream* fs_ptr) const;
        uint32_t dumpLatencyHistogram_(std::fstream* fs_ptr) const;
        uint32_t dumpPerclientworkerReadcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpPerclientworkerWritecnts_(std::fstream* fs_ptr) const;

        // Load per-client statistics for TotalStatisticsTracker
        uint32_t load_(const std::string& filepath);

        // Used by load_() to load per-client statistics
        uint32_t loadPerclientWorkercnt_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerLocalHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerCooperativeHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerReqcnts_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogramSize_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogram_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerReadcnts_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerWritecnts_(std::fstream* fs_ptr);

        // (A) Const shared variables

        // ClientStatisticsWrapper only uses client index to specify instance_name_ -> no need to maintain client_idx_
        std::string instance_name_;

        const bool allow_update_; // NOT allow statistics update when aggregation (by statistics_aggregator)
        //uint32_t perclient_workercnt_; // Come from Param
        //uint32_t latency_histogram_size_; // Come from Config::latency_histogram_size_

        // (B) Non-const individual variables (latency_histogram_ is shared)
        // NOTE: we have to use dynamic array for std::atomic<uint32_t>, as it does NOT have copy constructor and operator= for std::vector (e.g., resize() and push_back())

        // (B.1) For updated intermediate statistics
        // Accessed by both client workers and client wrapper
        std::atomict<uint32_t> cur_slot_idx_; // Index of current time slot
        std::atomic<uint32_t>* slot0_perclientworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* slot0_perclientworker_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        std::atomic<uint32_t>* slot0_perclientworker_reqcnts_;
        std::atomic<uint32_t>* slot0_latency_histogram_; // thread safe
        std::atomic<uint32_t>* slot0_perclientworker_readcnts_;
        std::atomic<uint32_t>* slot0_perclientworker_writecnts_;

        // (B.2) For updated stable statistics (stable_latency_histogram_ will NOT be aggregated)
        // Accessed by both client workers and client wrapper
        std::atomic<uint32_t>* stable_perclientworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* stable_perclientworker_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        std::atomic<uint32_t>* stable_perclientworker_reqcnts_;
        std::atomic<uint32_t>* stable_latency_histogram_; // thread safe
        std::atomic<uint32_t>* stable_perclientworker_readcnts_;
        std::atomic<uint32_t>* stable_perclientworker_writecnts_;

        // (C) For aggregated intermediate statistics
        // ONLY accessed by client wrapper
        std::vector<uint32_t> intermediate_total_local_hitcnts;
        std::vector<uint32_t> intermediate_total_cooperative_hitcnts;
        std::vector<uint32_t> intermediate_total_reqcnts;
        std::vector<uint32_t> intermediate_avg_latencies_;
        std::vector<uint32_t> intermediate_min_latencies_;
        std::vector<uint32_t> intermediate_medium_latencies_;
        std::vector<uint32_t> intermediate_tail90_latencies_;
        std::vector<uint32_t> intermediate_tail95_latencies_;
        std::vector<uint32_t> intermediate_tail99_latencies_;
        std::vector<uint32_t> intermediate_max_latencies_;
        std::vector<uint32_t> intermediate_total_readcnts_;
        std::vector<uint32_t> intermediate_total_writecnts_;
        // TODO

        // (D) For aggregated stable statistics
        uint32_t stable_total_local_hitcnt_;
        uint32_t stable_total_cooperative_hitcnt_;
        uint32_t stable_total_reqcnt_;
        uint32_t stable_total_readcnt_;
        uint32_t stable_total_writecnt_;
    };
}

#endif