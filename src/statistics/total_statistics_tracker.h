/*
 * TotalStatisticsTracker: track aggregate statistics from all clients.
 * 
 * By Siyuan Sheng (2023.05.25).
 */

#ifndef TOTAL_STATISTICS_TRACKER_H
#define TOTAL_STATISTICS_TRACKER_H

#include <string>

#include "statistics/client_statistics_tracker.h"

namespace covered
{
    class TotalStatisticsTracker
    {
    public:
        TotalStatisticsTracker(uint32_t clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs);
        ~TotalStatisticsTracker();

        // Get aggregate statistics related with hit ratio
        uint32_t getTotalLocalHitcnt() const;
        uint32_t getTotalCooperativeHitcnt() const;
        uint32_t getTotalReqcnt() const;

        // Get aggregate statistics related with latency
        uint32_t getAvgLatency() const;
        uint32_t getMinLatency() const;
        uint32_t getMediumLatency() const;
        uint32_t getTail90Latency() const;
        uint32_t getTail95Latency() const;
        uint32_t getTail99Latency() const;
        uint32_t getMaxLatency() const;

        // Get string for aggregate statistics
        std::string toString() const;
    private:
        static const std::string kClassName;

        void preprocessClientStatistics_(uint32_t clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs);
        void aggregateClientStatistics_();

        // Pre-processed statistics
        uint32_t* perclient_local_hitcnts_; // Hit local edge cache of closest edge node
        uint32_t* perclient_cooperative_hitcnts_; // Hit cooperative edge cache of some target edge node
        uint32_t* perclient_reqcnts_;
        uint32_t* latency_histogram_;
        uint32_t clientcnt_; // Come from Util::Param
        uint32_t latency_histogram_size_; // Come from Util::Config

        // Aggregate statistics
        uint32_t total_local_hitcnt_;
        uint32_t total_cooperative_hitcnt_;
        uint32_t total_reqcnt_;
        uint32_t avg_latency_;
        uint32_t min_latency_;
        uint32_t medium_latency_;
        uint32_t tail90_latency_;
        uint32_t tail95_latency_;
        uint32_t tail99_latency_;
        uint32_t max_latency_;
    };
}

#endif