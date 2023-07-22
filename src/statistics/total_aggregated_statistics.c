#include "statistics/total_aggregated_statistics.h"

namespace covered
{
    const std::string TotalAggregatedStatistics::kClassName("TotalAggregatedStatistics");

    TotalAggregatedStatistics::TotalAggregatedStatistics(std::vector<ClientAggregatedStatistics*> client_aggregated_statistics_ptrs) : AggregatedStatisticsBase()
    {
        assert(client_aggregated_statistics_ptrs.size() > 0);

        aggregateClientAggregatedStatistics_(client_aggregated_statistics_ptrs);
    }
    
    TotalAggregatedStatistics::~TotalAggregatedStatistics() {}

    void TotalAggregatedStatistics::aggregateClientAggregatedStatistics_(std::vector<ClientAggregatedStatistics*> client_aggregated_statistics_ptrs)
    {
        const uint32_t clientcnt = client_aggregated_statistics_ptrs.size();
        assert(clientcnt > 0);

        // Aggregate ClientAggregatedStatistics of all clients
        uint64_t total_avg_latency = 0; // Avoid integer overflow
        std::vector<uint32_t medium_latencies;
        std::vector<uint32_t tail90_latencies;
        std::vector<uint32_t tail95_latencies;
        std::vector<uint32_t tail99_latencies;
        for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
        {
            ClientAggregatedStatistics tmp_client_aggregated_statistics_ptr = client_aggregated_statistics_ptrs[clientidx];
            assert(tmp_client_aggregated_statistics_ptr != NULL);

            // Aggregate hit ratio statistics
            total_local_hitcnt_ += tmp_client_aggregated_statistics_ptr->total_local_hitcnt_;
            total_cooperative_hitcnt_ += tmp_client_aggregated_statistics_ptr->total_cooperative_hitcnt_;
            total_reqcnt_ += tmp_client_aggregated_statistics_ptr->total_reqcnt_;

            // Pre-process latency statistics
            total_avg_latency += tmp_client_aggregated_statistics_ptr->avg_latency_;
            if (min_latency_ == 0 || min_latency_ > tmp_client_aggregated_statistics_ptr->min_latency_)
            {
                min_latency_ = tmp_client_aggregated_statistics_ptr->min_latency_;
            }
            medium_latencies.push_back(tmp_client_aggregated_statistics_ptr->medium_latency_);
            tail90_latencies.push_back(tmp_client_aggregated_statistics_ptr->tail90_latencies);
            tail95_latencies.push_back(tmp_client_aggregated_statistics_ptr->tail95_latencies);
            tail99_latencies.push_back(tmp_client_aggregated_statistics_ptr->tail99_latencies);
            if (max_latency == 0 || max_latency < tmp_client_aggregated_statistics_ptr->max_latency_)
            {
                max_latency_ = tmp_client_aggregated_statistics_ptr->max_latency_;
            }

            // Aggregate read-write ratio statistics
            total_readcnt_ += tmp_client_aggregated_statistics_ptr->total_readcnt_;
            total_writecnt_ += tmp_client_aggregated_statistics_ptr->total_writecnt_;
        }

        // Aggregate latency statistics approximately
        avg_latency_ = static_cast<uint32_t>(total_avg_latency / clientcnt);
        std::sort(medium_latencies.begin(), medium_latencies.end(), std::less<uint32_t>); // ascending order
        medium_latency_ = medium_latencies[clientcnt * 0.5];
        std::sort(tail90_latencies.begin(), tail90_latencies.end(), std::less<uint32_t>); // ascending order
        tail90_latency_ = tail90_latencies[clientcnt * 0.9];
        std::sort(tail95_latencies.begin(), tail95_latencies.end(), std::less<uint32_t>); // ascending order
        tail95_latency_ = tail95_latencies[clientcnt * 0.95];
        std::sort(tail99_latencies.begin(), tail99_latencies.end(), std::less<uint32_t>); // ascending order
        tail99_latency_ = tail99_latencies[clientcnt * 0.99];

        return;
    }
}