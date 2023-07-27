/*
 * TotalAggregatedStatistics: store total aggregated statistics (aggregate ClientAggregatedStatistics of all clients).
 *
 * NOTE: TotalAggregatedStatistics aggregates latency statistics approximately to avoid holding too many latency histograms.
 *
 * NOTE: TotalAggregatedStatistics does NOT support online updates.
 * 
 * By Siyuan Sheng (2023.07.22).
 */

#ifndef TOTAL_AGGREGATED_STATISTICS_H
#define TOTAL_AGGREGATED_STATISTICS_H

#include <string>
#include <vector>

#include "statistics/aggregated_statistics_base.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    class TotalAggregatedStatistics : public AggregatedStatisticsBase
    {
    public:
        TotalAggregatedStatistics();
        TotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics);
        ~TotalAggregatedStatistics();
    private:
        static const std::string kClassName;

        void aggregateClientAggregatedStatistics_(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics);
    };
}

#endif