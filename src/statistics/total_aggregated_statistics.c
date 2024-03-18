#include "statistics/total_aggregated_statistics.h"

#include <algorithm> // sort
#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string TotalAggregatedStatistics::kClassName("TotalAggregatedStatistics");

    TotalAggregatedStatistics::TotalAggregatedStatistics() : AggregatedStatisticsBase(), sec_(0)
    {
    }

    TotalAggregatedStatistics::TotalAggregatedStatistics(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics, const uint32_t& sec) : AggregatedStatisticsBase()
    {
        assert(curslot_perclient_aggregated_statistics.size() > 0);

        aggregateClientAggregatedStatistics_(curslot_perclient_aggregated_statistics);

        assert(sec > 0);
        sec_ = sec;
    }
    
    TotalAggregatedStatistics::~TotalAggregatedStatistics() {}

    std::string TotalAggregatedStatistics::toString() const
    {
        std::ostringstream oss;

        oss << AggregatedStatisticsBase::toString();

        oss << "[Throughput Statistics]" << std::endl;
        oss << "time: " << sec_ << " seconds" << std::endl;
        const double throughput = static_cast<double>(total_reqcnt_) / static_cast<double>(sec_);
        oss << "throughput: " << throughput << " OPS" << std::endl;

        // Dump markdown string to help collect statistics
        oss << "[Markdown Format]" << std::endl;
        oss << "| Global Hit Ratio (%) (Local + Cooperative) | Avg Latency (ms) | Thpt (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |" << std::endl;
        oss << "| " << getTotalObjectHitRatio() * 100 << " (" << getLocalObjectHitRatio() * 100 << " + " << getCooperativeObjectHitRatio() * 100 << ")" << " | " << static_cast<double>(avg_latency_) / 1000.0 << " | " << throughput << " | " << B2MB(static_cast<double>(total_bandwidth_usage_.getCrossEdgeBandwidthBytes()) / static_cast<double>(total_reqcnt_)) << "/" << B2MB(static_cast<double>(total_bandwidth_usage_.getEdgeCloudBandwidthBytes()) / static_cast<double>(total_reqcnt_)) << " | " << static_cast<double>(total_bandwidth_usage_.getCrossEdgeMsgcnt()) / static_cast<double>(total_reqcnt_) << "/" << static_cast<double>(total_bandwidth_usage_.getEdgeCloudMsgcnt()) / static_cast<double>(total_reqcnt_) << " |" << std::endl;

        return oss.str();
    }

    uint32_t TotalAggregatedStatistics::getAggregatedStatisticsIOSize()
    {
        // AggregatedStatisticsBase + sec_
        return AggregatedStatisticsBase::getAggregatedStatisticsIOSize() + sizeof(uint32_t);
    }

    uint32_t TotalAggregatedStatistics::serialize(DynamicArray& dynamic_array, const uint32_t& position) const
    {
        uint32_t size = position;

        // Serialize AggregatedStatisticsBase
        uint32_t aggregated_statistics_base_serialize_size = AggregatedStatisticsBase::serialize(dynamic_array, size);
        size += aggregated_statistics_base_serialize_size;

        // Serialize sec_
        dynamic_array.deserialize(size, (const char*)&sec_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        return size - position;
    }

    uint32_t TotalAggregatedStatistics::deserialize(const DynamicArray& dynamic_array, const uint32_t& position)
    {
        uint32_t size = position;

        // Deserialize AggregatedStatisticsBase
        uint32_t aggregated_statistics_base_deserialize_size = AggregatedStatisticsBase::deserialize(dynamic_array, size);
        size += aggregated_statistics_base_deserialize_size;

        // Deserialize sec_
        dynamic_array.serialize(size, (char*)&sec_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        return size - position;
    }

    void TotalAggregatedStatistics::aggregateClientAggregatedStatistics_(const std::vector<ClientAggregatedStatistics>& curslot_perclient_aggregated_statistics)
    {
        const uint32_t clientcnt = curslot_perclient_aggregated_statistics.size();
        assert(clientcnt > 0);

        // Aggregate ClientAggregatedStatistics of all clients
        uint64_t total_avg_latency = 0; // Avoid integer overflow
        std::vector<uint32_t> medium_latencies;
        std::vector<uint32_t> tail90_latencies;
        std::vector<uint32_t> tail95_latencies;
        std::vector<uint32_t> tail99_latencies;
        for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
        {
            const ClientAggregatedStatistics& tmp_client_aggregated_statistics = curslot_perclient_aggregated_statistics[clientidx];

            // Aggregate object hit ratio statistics
            total_local_hitcnt_ += tmp_client_aggregated_statistics.total_local_hitcnt_;
            total_cooperative_hitcnt_ += tmp_client_aggregated_statistics.total_cooperative_hitcnt_;
            total_reqcnt_ += tmp_client_aggregated_statistics.total_reqcnt_;

            // Aggregate byte hit ratio statistics
            total_local_hitbytes_ += tmp_client_aggregated_statistics.total_local_hitbytes_;
            total_cooperative_hitbytes_ += tmp_client_aggregated_statistics.total_cooperative_hitbytes_;
            total_reqbytes_ += tmp_client_aggregated_statistics.total_reqbytes_;

            // Pre-process latency statistics
            total_avg_latency += tmp_client_aggregated_statistics.avg_latency_;
            if (min_latency_ == 0 || min_latency_ > tmp_client_aggregated_statistics.min_latency_)
            {
                min_latency_ = tmp_client_aggregated_statistics.min_latency_;
            }
            medium_latencies.push_back(tmp_client_aggregated_statistics.medium_latency_);
            tail90_latencies.push_back(tmp_client_aggregated_statistics.tail90_latency_);
            tail95_latencies.push_back(tmp_client_aggregated_statistics.tail95_latency_);
            tail99_latencies.push_back(tmp_client_aggregated_statistics.tail99_latency_);
            if (max_latency_ == 0 || max_latency_ < tmp_client_aggregated_statistics.max_latency_)
            {
                max_latency_ = tmp_client_aggregated_statistics.max_latency_;
            }

            // Aggregate read-write ratio statistics
            total_readcnt_ += tmp_client_aggregated_statistics.total_readcnt_;
            total_writecnt_ += tmp_client_aggregated_statistics.total_writecnt_;

            // Aggregate cache utilization statistics
            total_cache_size_bytes_ += tmp_client_aggregated_statistics.total_cache_size_bytes_;
            total_cache_capacity_bytes_ += tmp_client_aggregated_statistics.total_cache_capacity_bytes_;

            // Aggregate workload key-value size statistics
            total_workload_key_size_ += tmp_client_aggregated_statistics.total_workload_key_size_;
            total_workload_value_size_ += tmp_client_aggregated_statistics.total_workload_value_size_;

            // Aggregate bandwidth usage statistics
            total_bandwidth_usage_.update(tmp_client_aggregated_statistics.total_bandwidth_usage_);
        }

        // Aggregate latency statistics approximately
        // TODO: Update ClientWrapper to return cur-slot client raw statistics to evaluator for accurate statistics aggregation
        avg_latency_ = static_cast<uint32_t>(total_avg_latency / clientcnt);
        std::sort(medium_latencies.begin(), medium_latencies.end(), std::less<uint32_t>()); // ascending order
        medium_latency_ = medium_latencies[clientcnt * 0.5];
        std::sort(tail90_latencies.begin(), tail90_latencies.end(), std::less<uint32_t>()); // ascending order
        tail90_latency_ = tail90_latencies[clientcnt * 0.9];
        std::sort(tail95_latencies.begin(), tail95_latencies.end(), std::less<uint32_t>()); // ascending order
        tail95_latency_ = tail95_latencies[clientcnt * 0.95];
        std::sort(tail99_latencies.begin(), tail99_latencies.end(), std::less<uint32_t>()); // ascending order
        tail99_latency_ = tail99_latencies[clientcnt * 0.99];

        return;
    }
}