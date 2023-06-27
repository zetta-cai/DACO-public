/*
 * ClientStatisticsTracker: track and dump per-client statistics.
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
        ClientStatisticsTracker(uint32_t perclient_workercnt, uint32_t latency_histogram_size, const uint32_t& client_idx);
        ClientStatisticsTracker(const std::string& filepath, const uint32_t& client_idx);
        ~ClientStatisticsTracker();

        // Update per-client hit ratio statistics
        void updateLocalHitcnt(const uint32_t& local_client_worker_idx);
        void updateCooperativeHitcnt(const uint32_t& local_client_worker_idx);
        void updateReqcnt(const uint32_t& local_client_worker_idx);

        // Update per-client latency statistics
        void updateLatency(const uint32_t& latency_us);

        // Dump per-client statistics for TotalStatisticsTracker
        uint32_t dump(const std::string& filepath) const;

        // Get per-client statistics for aggregation
        std::atomic<uint32_t>* getPerclientworkerLocalHitcnts() const;
        std::atomic<uint32_t>* getPerclientworkerCooperativeHitcnts() const;
        std::atomic<uint32_t>* getPerclientworkerReqcnts() const;
        std::atomic<uint32_t>* getLatencyHistogram() const;
        uint32_t getPerclientWorkercnt() const;
        uint32_t getLatencyHistogramSize() const;
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

        // Load per-client statistics for TotalStatisticsTracker
        uint32_t load_(const std::string& filepath);

        // Used by load_() to load per-client statistics
        uint32_t loadPerclientWorkercnt_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerLocalHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerCooperativeHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerclientworkerReqcnts_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogramSize_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogram_(std::fstream* fs_ptr);

        // ClientStatisticsWrapper only uses client index to specify instance_name_ -> no need to maintain client_idx_
        std::string instance_name_;

        // NOTE: we have to use dynamic array for std::atomic<uint32_t>, as it does NOT have default constructor and copy constructor
        std::atomic<uint32_t>* perclientworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* perclientworker_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        std::atomic<uint32_t>* perclientworker_reqcnts_;
        std::atomic<uint32_t>* latency_histogram_;
        uint32_t perclient_workercnt_; // Come from Util::Param
        uint32_t latency_histogram_size_; // Come from Util::Config
    };
}

#endif