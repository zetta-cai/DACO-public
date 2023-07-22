/*
 * ClientUpdatedStatistics: store statistics of client workers for online updates.
 * 
 * By Siyuan Sheng (2023.07.22).
 */

#ifndef CLIENT_UPDATED_STATISTICS_H
#define CLIENT_UPDATED_STATISTICS_H

#include <atomic>
#include <string>

namespace covered
{
    class ClientUpdatedStatistics
    {
    public:
        ClientUpdatedStatistics(uint32_t perclient_workercnt, const uint32_t& client_idx);
        ~ClientUpdatedStatistics();

        // Update hit ratio statistics of a client worker
        void updateLocalHitcnt(const uint32_t& local_client_worker_idx);
        void updateCooperativeHitcnt(const uint32_t& local_client_worker_idx);
        void updateReqcnt(const uint32_t& local_client_worker_idx);

        // Update latency statistics of a client worker
        void updateLatency(const uint32_t& latency_us);

        // Update read-write statistics of a client worker
        void updateReadcnt(const uint32_t& local_client_worker_idx);
        void updateWritecnt(const uint32_t& local_client_worker_idx);

        friend class ClientAggregatedStatistics; // To aggregate per-client-worker statistics for a client
    private:
        static const std::string kClassName;

        // Const variables
        std::string instance_name_;
        uint32_t perclient_workercnt_; // Come from Param
        uint32_t latency_histogram_size_; // Come from Config::latency_histogram_size_

        // Per-client-worker hit ratio statistics
        std::atomic<uint32_t>* perclientworker_local_hitcnts_; // Hit local edge cache of closest edge node
        std::atomic<uint32_t>* perclientworker_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        std::atomic<uint32_t>* perclientworker_reqcnts_;

        // Per-client-worker latency statistics
        std::atomic<uint32_t>* latency_histogram_; // thread safe

        // Per-client-worker read-write statistics
        std::atomic<uint32_t>* perclientworker_readcnts_;
        std::atomic<uint32_t>* perclientworker_writecnts_;
    };
}

#endif