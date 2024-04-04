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

#include "common/bandwidth_usage.h"
#include "common/dynamic_array.h"

namespace covered
{
    class AggregatedStatisticsBase
    {
    public:
        AggregatedStatisticsBase();
        ~AggregatedStatisticsBase();

        // Get aggregate statistics related with object hit ratio
        uint32_t getTotalLocalHitcnt() const;
        uint32_t getTotalCooperativeHitcnt() const;
        uint32_t getTotalReqcnt() const;
        // Calculate object hit ratio based on aggregated statistics
        double getLocalObjectHitRatio() const;
        double getCooperativeObjectHitRatio() const;
        double getTotalObjectHitRatio() const;

        // Get aggregated statistics related with byte hit ratio
        double getTotalLocalHitbytes() const;
        double getTotalCooperativeHitbytes() const;
        double getTotalReqbytes() const;
        // Calculate byte hit ratio based on aggregated statistics
        double getLocalByteHitRatio() const;
        double getCooperativeByteHitRatio() const;
        double getTotalByteHitRatio() const;

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

        // Get aggregated statistics related with cache utilization
        uint64_t getTotalCacheSizeBytes() const;
        uint64_t getTotalCacheCapacityBytes() const;
        uint64_t getTotalCacheMarginBytes() const; // cache size bytes + cache margin bytes = cache capacity bytes
        double getTotalCacheUtilization() const;

        // Get aggregated statistics related with workload key-value size
        double getLocalHitWorkloadKeySize() const;
        double getCooperativeHitWorkloadKeySize() const;
        double getGlobalMissWorkloadKeySize() const;
        double getTotalWorkloadKeySize() const;
        double getLocalHitWorkloadValueSize() const;
        double getCooperativeHitWorkloadValueSize() const;
        double getGlobalMissWorkloadValueSize() const;
        double getTotalWorkloadValueSize() const;
        double getAvgLocalHitWorkloadKeySize() const;
        double getAvgCooperativeHitWorkloadKeySize() const;
        double getAvgGlobalMissWorkloadKeySize() const;
        double getAvgWorkloadKeySize() const;
        double getAvgLocalHitWorkloadValueSize() const;
        double getAvgCooperativeHitWorkloadValueSize() const;
        double getAvgGlobalMissWorkloadValueSize() const;
        double getAvgWorkloadValueSize() const;

        // Get aggregated statistics related with bandwidth usage
        BandwidthUsage getTotalBandwidthUsage() const;

        // Get string for aggregate statistics
        std::string toString() const;

        // Used by ClientAggregatedStatistics for network I/O (message payload); used by TotalAggregatedStatsitics for disk I/O (file content)
        static uint32_t getAggregatedStatisticsIOSize(); // For file I/O (dump/load) and network I/O (message payload)
        uint32_t serialize(DynamicArray& dynamic_array, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& dynamic_array, const uint32_t& position);

        const AggregatedStatisticsBase& operator=(const AggregatedStatisticsBase& other);
    private:
        static const std::string kClassName;
    protected:
        // Aggregated statistics related with object hit ratio
        uint32_t total_local_hitcnt_;
        uint32_t total_cooperative_hitcnt_;
        uint32_t total_reqcnt_;

        // Aggregated statistics related with byte hit ratio
        double total_local_hitbytes_;
        double total_cooperative_hitbytes_;
        double total_reqbytes_;

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

        // Aggregated statistics related with cache utilization
        uint64_t total_cache_size_bytes_;
        uint64_t total_cache_capacity_bytes_;

        // Aggregated statistics related with workload key-value size
        double localhit_workload_key_size_;
        double cooperativehit_workload_key_size_;
        double globalmiss_workload_key_size_;
        double total_workload_key_size_;
        double localhit_workload_value_size_;
        double cooperativehit_workload_value_size_;
        double globalmiss_workload_value_size_;
        double total_workload_value_size_;

        // Aggregated statistics related with bandwidth usage
        BandwidthUsage total_bandwidth_usage_;
    };
}

#endif