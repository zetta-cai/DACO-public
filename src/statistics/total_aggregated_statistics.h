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
        TotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics, const uint32_t& sec);
        ~TotalAggregatedStatistics();

        // Get string for aggregate statistics
        std::string toString() const; // NOTE: overwrite AggregatedStatisticsBase::toString() for throughput based on sec_

        // NOTE: overwrite AggregatedStatisticsBase::getAggregatedStatisticsIOSize/serialize/deserialize for sec_
        static uint32_t getAggregatedStatisticsIOSize();
        uint32_t serialize(DynamicArray& dynamic_array, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& dynamic_array, const uint32_t& position);
    private:
        static const std::string kClassName;

        void aggregateClientAggregatedStatistics_(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics);

        uint32_t sec_;
    };
}

#endif