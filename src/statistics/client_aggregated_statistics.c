#include "statistics/client_aggregated_statistics.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string ClientAggregatedStatistics::kClassName("ClientAggregatedStatistics");

    ClientAggregatedStatistics::ClientAggregatedStatistics() : AggregatedStatisticsBase()
    {
    }

    ClientAggregatedStatistics::ClientAggregatedStatistics(ClientRawStatistics* client_raw_statistics_ptr) : AggregatedStatisticsBase()
    {
        assert(client_raw_statistics_ptr != NULL);

        aggregateClientRawStatistics_(client_raw_statistics_ptr);
    }

    ClientAggregatedStatistics::ClientAggregatedStatistics(const AggregatedStatisticsBase& base_instance)
    {
        AggregatedStatisticsBase::operator=(base_instance);
    }

    ClientAggregatedStatistics::~ClientAggregatedStatistics() {}

    void ClientAggregatedStatistics::aggregateClientRawStatistics_(ClientRawStatistics* client_raw_statistics_ptr)
    {
        assert(client_raw_statistics_ptr != NULL);
        client_raw_statistics_ptr->checkPointers_();

        const uint32_t perclient_workercnt = client_raw_statistics_ptr->perclient_workercnt_;
        const uint32_t latency_histogram_size = client_raw_statistics_ptr->latency_histogram_size_;

        // Aggregate ClientRawStatistics of client workers in a client

        // Aggregate per-client-worker hit ratio statistics
        for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
        {
            total_local_hitcnt_ += client_raw_statistics_ptr->perclientworker_local_hitcnts_[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            total_cooperative_hitcnt_ += client_raw_statistics_ptr->perclientworker_cooperative_hitcnts_[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            total_reqcnt_ += client_raw_statistics_ptr->perclientworker_reqcnts_[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
        }

        // Aggregate per-client latency statistics accurately
        uint32_t total_latency_cnt = 0; // i.e., total reqcnt
        for (uint32_t latency_us = 0; latency_us < latency_histogram_size; latency_us++)
        {
            uint32_t tmp_latency_cnt = client_raw_statistics_ptr->latency_histogram_[latency_us];
            total_latency_cnt += tmp_latency_cnt;
        }
        // Get latency statistics
        uint32_t cur_latency_cnt = 0;
        uint64_t tmp_avg_latency = 0; // To avoid integer overflow
        for (uint32_t latency_us = 0; latency_us < latency_histogram_size; latency_us++)
        {
            uint32_t tmp_latency_cnt = client_raw_statistics_ptr->latency_histogram_[latency_us];
            cur_latency_cnt += tmp_latency_cnt;
            double cur_ratio = 0.0;
            if (total_latency_cnt != 0)
            {
                cur_ratio = static_cast<double>(cur_latency_cnt) / static_cast<double>(total_latency_cnt);
            }

            tmp_avg_latency += static_cast<uint64_t>(latency_us * tmp_latency_cnt);
            if (min_latency_ == 0 && tmp_latency_cnt > 0)
            {
                min_latency_ = latency_us;
            }
            if (medium_latency_ == 0 && cur_ratio >= 0.5)
            {
                medium_latency_ = latency_us;
            }
            if (tail90_latency_ == 0 && cur_ratio >= 0.9)
            {
                tail90_latency_ = latency_us;
            }
            if (tail95_latency_ == 0 && cur_ratio >= 0.95)
            {
                tail95_latency_ = latency_us;
            }
            if (tail99_latency_ == 0 && cur_ratio >= 0.99)
            {
                tail99_latency_ = latency_us;
            }
            if (tmp_latency_cnt > 0 && (max_latency_ == 0 || latency_us > max_latency_))
            {
                max_latency_ = latency_us;
            }
        }
        if (total_latency_cnt != 0)
        {
            tmp_avg_latency /= total_latency_cnt;
        }
        assert(tmp_avg_latency >= 0 && tmp_avg_latency < latency_histogram_size);
        avg_latency_ = static_cast<uint32_t>(tmp_avg_latency);

        // Aggregate per-client read-write ratio statistics
        for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
        {
            total_readcnt_ += client_raw_statistics_ptr->perclientworker_readcnts_[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            total_writecnt_ += client_raw_statistics_ptr->perclientworker_writecnts_[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
        }

        // Copy closest edge cache utilization
        total_cache_size_bytes_ = client_raw_statistics_ptr->closest_edge_cache_size_bytes_;
        total_cache_capacity_bytes_ = client_raw_statistics_ptr->closest_edge_cache_capacity_bytes_;

        return;
    }

    const ClientAggregatedStatistics& ClientAggregatedStatistics::operator=(const ClientAggregatedStatistics& other)
    {
        AggregatedStatisticsBase::operator=(other);

        return *this;
    }
}