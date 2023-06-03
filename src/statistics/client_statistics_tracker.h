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
        ClientStatisticsTracker(uint32_t perclient_workercnt, uint32_t latency_histogram_size);
        ClientStatisticsTracker(const std::string& filepath);
        ~ClientStatisticsTracker();

        // Update per-client hit ratio statistics
        void updateLocalHitcnt(const uint32_t& local_worker_index);
        void updateCooperativeHitcnt(const uint32_t& local_worker_index);
        void updateReqcnt(const uint32_t& local_worker_index);

        // Update per-client latency statistics
        void updateLatency(const uint32_t& local_worker_index, const uint32_t& latency_us);

        // Dump per-client statistics for TotalStatisticsTracker
        uint32_t dump(const std::string& filepath) const;

        // Get per-client statistics for aggregation
        std::atomic<uint32_t>* getPerworkerLocalHitcnts() const;
        std::atomic<uint32_t>* getPerworkerCooperativeHitcnts() const;
        std::atomic<uint32_t>* getPerworkerReqcnts() const;
        std::atomic<uint32_t>* getLatencyHistogram() const;
        uint32_t getPerclientWorkercnt() const;
        uint32_t getLatencyHistogramSize() const;
    private:
        static const std::string kClassName;

        // Used by dump() to check filepath and dump per-client statistics
        std::string checkFilepathForDump_(const std::string& filepath) const;
        uint32_t dumpPerclientWorkercnt_(std::fstream* fs_ptr) const;
        uint32_t dumpPerworkerLocalHitcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpPerworkerCooperativeHitcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpPerworkerReqcnts_(std::fstream* fs_ptr) const;
        uint32_t dumpLatencyHistogramSize_(std::fstream* fs_ptr) const;
        uint32_t dumpLatencyHistogram_(std::fstream* fs_ptr) const;

        // Load per-client statistics for TotalStatisticsTracker
        uint32_t load_(const std::string& filepath);

        // Used by load_() to load per-client statistics
        uint32_t loadPerclientWorkercnt_(std::fstream* fs_ptr);
        uint32_t loadPerworkerLocalHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerworkerCooperativeHitcnts_(std::fstream* fs_ptr);
        uint32_t loadPerworkerReqcnts_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogramSize_(std::fstream* fs_ptr);
        uint32_t loadLatencyHistogram_(std::fstream* fs_ptr);

        std::atomic<uint32_t>* perworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* perworker_cooperative_hitcnts_; // Hit cooperative edge cache of neighbor edge node
        std::atomic<uint32_t>* perworker_reqcnts_;
        std::atomic<uint32_t>* latency_histogram_;
        uint32_t perclient_workercnt_; // Come from Util::Param
        uint32_t latency_histogram_size_; // Come from Util::Config
    };
}

#endif