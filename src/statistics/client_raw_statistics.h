/*
 * ClientRawStatistics: store statistics of client workers for online updates.
 * 
 * By Siyuan Sheng (2023.07.22).
 */

#ifndef CLIENT_RAW_STATISTICS_H
#define CLIENT_RAW_STATISTICS_H

#include <atomic>
#include <string>

namespace covered
{
    class ClientRawStatistics
    {
    public:
        ClientRawStatistics(uint32_t perclient_workercnt);
        ~ClientRawStatistics();

        friend class ClientStatisticsTracker; // To update cur-slot/stable client raw statistics of client workers
        friend class ClientAggregatedStatistics; // To aggregate cur-slot/stable client raw statistics of client workers
    private:
        static const std::string kClassName;

        // Update hit ratio statistics of a client worker
        void updateLocalHitcnt_(const uint32_t& local_client_worker_idx);
        void updateCooperativeHitcnt_(const uint32_t& local_client_worker_idx);
        void updateReqcnt_(const uint32_t& local_client_worker_idx);

        // Update latency statistics of a client worker
        void updateLatency_(const uint32_t& latency_us);

        // Update read-write ratio statistics of a client worker
        void updateReadcnt_(const uint32_t& local_client_worker_idx);
        void updateWritecnt_(const uint32_t& local_client_worker_idx);

        // Clean client raw statistics
        void clean();

        void checkPointers_() const;

        // Const variables
        uint32_t perclient_workercnt_; // Come from Param
        uint32_t latency_histogram_size_; // Come from Config::latency_histogram_size_

        // Per-client-worker hit ratio statistics
        std::atomic<uint32_t>* perclientworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* perclientworker_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        std::atomic<uint32_t>* perclientworker_reqcnts_;

        // Per-client-worker latency statistics
        std::atomic<uint32_t>* latency_histogram_; // thread safe

        // Per-client-worker read-write ratio statistics
        std::atomic<uint32_t>* perclientworker_readcnts_;
        std::atomic<uint32_t>* perclientworker_writecnts_;
    };
}

#endif