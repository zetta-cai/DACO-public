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
        TotalStatisticsTracker();
        TotalStatisticsTracker(const std::string& filepath);
        ~TotalStatisticsTracker();

        // Aggregate cur-slot client aggregated statistics into per-slot total aggregated statistics
        void updatePerslotTotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics);

        // Cache is stable if cache is filled up and total hit ratio converges
        bool isPerSlotTotalAggregatedStatisticsStable(double& cache_hit_ratio);

        uint32_t dump(const std::string filepath) const;

        // Get string for per-slot/stable total aggregate statistics
        std::string toString() const;
    private:
        static const std::string kClassName;

        // Used by dump() to check filepath and dump total aggregated statistics
        std::string checkFilepathForDump_(const std::string& filepath) const;

        // Load per-slot/total aggregated statistics
        void load_(const std::string& filepath);

        const bool allow_update_; // NOT allow statistics update for loaded total statistics

        // Per-slot/stable total aggregated statistics
        std::vector<TotalAggregatedStatistics> perslot_total_aggregated_statistics_;
        TotalAggregatedStatistics stable_total_aggregated_statistics_;
    };
}

#endif