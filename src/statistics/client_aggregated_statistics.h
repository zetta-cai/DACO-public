/*
 * ClientAggregatedStatistics: store client aggregated statistics (aggregate ClientRawStatistics of client workers in a client).
 *
 * NOTE: ClientAggregatedStatistics aggregate latency statistics accurately based on the latency histogram of the client.
 *
 * NOTE: ClientAggregatedStatistics does NOT support online updates.
 * 
 * By Siyuan Sheng (2023.07.22).
 */

#ifndef CLIENT_AGGREGATED_STATISTICS_H
#define CLIENT_AGGREGATED_STATISTICS_H

#include <string>

#include "statistics/aggregated_statistics_base.h"
#include "statistics/client_raw_statistics.h"

namespace covered
{
    class ClientAggregatedStatistics : public AggregatedStatisticsBase
    {
    public:
        ClientAggregatedStatistics();
        ClientAggregatedStatistics(ClientRawStatistics* client_raw_statistics_ptr);
        ClientAggregatedStatistics(const AggregatedStatisticsBase& base_instance);
        ~ClientAggregatedStatistics();

        const ClientAggregatedStatistics& operator=(const ClientAggregatedStatistics& other);

        friend class TotalAggregatedStatistics;
    private:
        static const std::string kClassName;

        void aggregateClientRawStatistics_(ClientRawStatistics* client_raw_statistics_ptr);
    };
}

#endif