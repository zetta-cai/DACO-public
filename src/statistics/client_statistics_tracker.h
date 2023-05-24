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
        ClientStatisticsTracker(uint32_t perclient_workercnt);
        ~ClientStatisticsTracker();

        // Update per-client hit ratio statistics
        void updateLocalHitcnt(const uint32_t& local_worker_index);
        void updateCooperativeHitcnt(const uint32_t& local_worker_index);
        void updateReqcnt(const uint32_t& local_worker_index);

        // Update per-client latency statistics
        void updateLatency(const uint32_t& local_worker_index), const uint32_t& latency_us);

        // Dump and load for TotalStatisticsTracker
        uint32_t dump(const std::string& filepath);
        uint32_t load(const std::string& filepath);
    private:
        static const std::string kClassName;

        std::atomic<uint32_t>* perworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* perworker_cooperative_hitcnts_; // Hit cooperative edge cache of neighbor edge node
        std::atomic<uint32_t>* perworker_reqcnts_;
        std::atomic<uint32_t>* latency_histogram_;
        uint32_t perclient_workercnt_;

    };
}

#endif