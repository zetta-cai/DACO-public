/*
 * AggregatedStatisticsBase: store client/total aggregated statistics.
 *
 * NOTE: AggregatedStatisticsBase does NOT support online updates.
 * 
 * By Siyuan Sheng (2023.07.22).
 */

#ifndef AGGREGATED_STATISTICS_BASE_H
#define AGGREGATED_STATISTICS_BASE_H

#include <string>

namespace covered
{
    class AggregatedStatisticsBase
    {
    public:
        AggregatedStatisticsBase();
        ~AggregatedStatisticsBase();

        // Get aggregate statistics related with hit ratio
        uint32_t getTotalLocalHitcnt() const;
        uint32_t getTotalCooperativeHitcnt() const;
        uint32_t getTotalReqcnt() const;
        // Calculate hit ratio based on aggregated statistics
        double getLocalHitRatio() const;
        double getCooperativeHitRatio() const;
        double getTotalHitRatio() const;

        // Get aggregate statistics related with latency
        uint32_t getAvgLatency() const;
        uint32_t getMinLatency() const;
        uint32_t getMediumLatency() const;
        uint32_t getTail90Latency() const;
        uint32_t getTail95Latency() const;
        uint32_t getTail99Latency() const;
        uint32_t getMaxLatency() const;

        // Get aggregate statistics related with read-write ratio
        uint32_t getTotalReadcnt() const;
        uint32_t getTotalWritecnt() const;

        // Get string for aggregate statistics
        std::string toString() const;

        const AggregatedStatisticsBase& operator=(const AggregatedStatisticsBase& other);
    private:
        static const std::string kClassName;

        // Aggregated statistics related with hit ratio
        uint32_t total_local_hitcnt_;
        uint32_t total_cooperative_hitcnt_;
        uint32_t total_reqcnt_;

        // Aggregated statistics related with latency
        uint32_t avg_latency_;
        uint32_t min_latency_;
        uint32_t medium_latency_;
        uint32_t tail90_latency_;
        uint32_t tail95_latency_;
        uint32_t tail99_latency_;
        uint32_t max_latency_;

        // Aggregated statistics related with read-write ratio
        uint32_t total_readcnt_;
        uint32_t total_writecnt_;
    };
}

#endif