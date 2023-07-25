/*
 * TotalStatisticsTracker: load per-slot/stable ClientAggregatedStatistics from all clients, aggregate ClientAggregatedStatistics into per-slot/stable TotalAggregatedStatistics, and print TotalAggregatedStatistics.
 * 
 * By Siyuan Sheng (2023.05.25).
 */

#ifndef TOTAL_STATISTICS_TRACKER_H
#define TOTAL_STATISTICS_TRACKER_H

#include <string>
#include <vector>

#include "statistics/client_statistics_tracker.h"
#include "statistics/total_aggregated_statistics.h"

namespace covered
{
    class TotalStatisticsTracker
    {
    public:
        TotalStatisticsTracker(const uint32_t& clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs);
        ~TotalStatisticsTracker();

        // Get string for per-slot/stable total aggregate statistics
        std::string toString() const;
    private:
        static const std::string kClassName;

        // Aggregate per-slot/stable client aggregated statistics into per-slot/stable total aggregated statistics
        void aggregateClientStatistics_(const uint32_t& clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs);

        // Per-slot/stable total aggregated statistics
        std::vector<TotalAggregatedStatistics> perslot_total_aggregated_statistics_;
        TotalAggregatedStatistics stable_total_aggregated_statistics_;
    };
}

#endif