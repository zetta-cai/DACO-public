#include "statistics/aggregated_statistics_base.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string AggregatedStatisticsBase::kClassName("AggregatedStatisticsBase");
    
    AggregatedStatisticsBase::AggregatedStatisticsBase()
    {
        total_local_hitcnt_ = 0;
        total_cooperative_hitcnt_ = 0;
        total_reqcnt_ = 0;

        total_local_hitbytes_ = 0.0;
        total_cooperative_hitbytes_ = 0.0;
        total_reqbytes_ = 0.0;

        avg_latency_ = 0;
        min_latency_ = 0;
        medium_latency_ = 0;
        tail90_latency_ = 0;
        tail95_latency_ = 0;
        tail99_latency_ = 0;
        max_latency_ = 0;

        total_readcnt_ = 0;
        total_writecnt_ = 0;

        total_cache_size_bytes_ = 0;
        total_cache_capacity_bytes_ = 0;

        total_workload_key_size_ = double(0.0);
        total_workload_value_size_ = double(0.0);

        total_bandwidth_usage_ = BandwidthUsage();
    }
        
    AggregatedStatisticsBase::~AggregatedStatisticsBase() {}

    // Get aggregate statistics related with object hit ratio

    uint32_t AggregatedStatisticsBase::getTotalLocalHitcnt() const
    {
        return total_local_hitcnt_;
    }
    
    uint32_t AggregatedStatisticsBase::getTotalCooperativeHitcnt() const
    {
        return total_cooperative_hitcnt_;
    }
    
    uint32_t AggregatedStatisticsBase::getTotalReqcnt() const
    {
        return total_reqcnt_;
    }

    double AggregatedStatisticsBase::getLocalObjectHitRatio() const
    {
        double local_hit_ratio = static_cast<double>(total_local_hitcnt_) / static_cast<double>(total_reqcnt_);
        return local_hit_ratio;
    }

    double AggregatedStatisticsBase::getCooperativeObjectHitRatio() const
    {
        double cooperative_hit_ratio = static_cast<double>(total_cooperative_hitcnt_) / static_cast<double>(total_reqcnt_);
        return cooperative_hit_ratio;
    }
    
    double AggregatedStatisticsBase::getTotalObjectHitRatio() const
    {
        return getLocalObjectHitRatio() + getCooperativeObjectHitRatio();
    }

    // Get aggregate statistics related with byte hit ratio

    double AggregatedStatisticsBase::getTotalLocalHitbytes() const
    {
        return total_local_hitbytes_;
    }

    double AggregatedStatisticsBase::getTotalCooperativeHitbytes() const
    {
        return total_cooperative_hitbytes_;
    }

    double AggregatedStatisticsBase::getTotalReqbytes() const
    {
        return total_reqbytes_;
    }

    double AggregatedStatisticsBase::getLocalByteHitRatio() const
    {
        double local_byte_hit_ratio = double(0.0);
        if (total_reqbytes_ > double(0.0))
        {
            local_byte_hit_ratio = total_local_hitbytes_ / total_reqbytes_;
        }
        return local_byte_hit_ratio;
    }

    double AggregatedStatisticsBase::getCooperativeByteHitRatio() const
    {
        double cooperative_byte_hit_ratio = double(0.0);
        if (total_reqbytes_ > double(0.0))
        {
            cooperative_byte_hit_ratio = total_cooperative_hitbytes_ / total_reqbytes_;
        }
        return cooperative_byte_hit_ratio;
    }

    double AggregatedStatisticsBase::getTotalByteHitRatio() const
    {
        return getLocalByteHitRatio() + getCooperativeByteHitRatio();
    }

    // Get aggregate statistics related with latency

    uint32_t AggregatedStatisticsBase::getAvgLatency() const
    {
        return avg_latency_;
    }
    
    uint32_t AggregatedStatisticsBase::getMinLatency() const
    {
        return min_latency_;
    }
    
    uint32_t AggregatedStatisticsBase::getMediumLatency() const
    {
        return medium_latency_;
    }

    uint32_t AggregatedStatisticsBase::getTail90Latency() const
    {
        return tail90_latency_;
    }

    uint32_t AggregatedStatisticsBase::getTail95Latency() const
    {
        return tail90_latency_;
    }

    uint32_t AggregatedStatisticsBase::getTail99Latency() const
    {
        return tail90_latency_;
    }
    
    uint32_t AggregatedStatisticsBase::getMaxLatency() const
    {
        return max_latency_;
    }

    // Get aggregate statistics related with read-write ratio

    uint32_t AggregatedStatisticsBase::getTotalReadcnt() const
    {
        return total_readcnt_;
    }

    uint32_t AggregatedStatisticsBase::getTotalWritecnt() const
    {
        return total_writecnt_;
    }

    // Get aggregated statistics related with cache utilization

    uint64_t AggregatedStatisticsBase::getTotalCacheSizeBytes() const
    {
        return total_cache_size_bytes_;
    }

    uint64_t AggregatedStatisticsBase::getTotalCacheCapacityBytes() const
    {
        return total_cache_capacity_bytes_;
    }

    uint64_t AggregatedStatisticsBase::getTotalCacheMarginBytes() const
    {
        if (total_cache_capacity_bytes_ >= total_cache_size_bytes_)
        {
            return total_cache_capacity_bytes_ - total_cache_size_bytes_;
        }
        else
        {
            return 0;
        }
    }

    double AggregatedStatisticsBase::getTotalCacheUtilization() const
    {
        double total_cache_utilization = static_cast<double>(total_cache_size_bytes_) / static_cast<double>(total_cache_capacity_bytes_);
        return total_cache_utilization;
    }

    // Get aggregated statistics related with workload key-value size

    double AggregatedStatisticsBase::getTotalWorkloadKeySize() const
    {
        return total_workload_key_size_;
    }
    
    double AggregatedStatisticsBase::getTotalWorkloadValueSize() const
    {
        return total_workload_value_size_;
    }

    double AggregatedStatisticsBase::getAvgWorkloadKeySize() const
    {
        double avg_workload_key_size = double(0.0);
        if (total_workload_key_size_ > double(0.0))
        {
            avg_workload_key_size = total_workload_key_size_ / static_cast<double>(total_reqcnt_);
        }
        return avg_workload_key_size;
    }

    double AggregatedStatisticsBase::getAvgWorkloadValueSize() const
    {
        double avg_workload_value_size = double(0.0);
        if (total_workload_value_size_ > double(0.0))
        {
            avg_workload_value_size = total_workload_value_size_ / static_cast<double>(total_reqcnt_);
        }
        return avg_workload_value_size;
    }

    // Get aggregated statistics related with bandwidth usage

    BandwidthUsage AggregatedStatisticsBase::getTotalBandwidthUsage() const
    {
        return total_bandwidth_usage_;
    }

    // Get string for aggregate statistics
    std::string AggregatedStatisticsBase::toString() const
    {
        std::ostringstream oss;

        oss << "[Object Hit Ratio Statistics]" << std::endl;
        oss << "total local hit cnts: " << total_local_hitcnt_ << std::endl;
        oss << "total cooperative hit cnts: " << total_cooperative_hitcnt_ << std::endl;
        oss << "total request cnts: " << total_reqcnt_ << std::endl;
        oss << "local object hit ratio: " << getLocalObjectHitRatio() << std::endl;
        oss << "cooperative object hit ratio: " << getCooperativeObjectHitRatio() << std::endl;
        oss << "total object hit ratio: " << getTotalObjectHitRatio() << std::endl;

        // NOTE: NO need weighted global object hit ratio: (i) each edge node has its own CoveredWeight; (ii) although we can use average CoveredWeight across all edge nodes, weighted global object hit ratio makes no sense (throughput is enough to verify the benefit of trade-off-aware cooperative caching in COVERED)
        // const double tmp_weight = static_cast<double>(CoveredWeight::getWeightInfo().getCooperativeHitWeight() / CoveredWeight::getWeightInfo().getLocalHitWeight()); // w2/w1
        // oss << "total object hit ratio (weighted by " << tmp_weight << "): " << getLocalObjectHitRatio() + tmp_weight * getCooperativeObjectHitRatio() << std::endl; // local object hit ratio + w2/w1 * cooperative object hit ratio

        oss << "[Byte Hit Ratio Statistics]" << std::endl;
        oss << "total local hit bytes: " << B2MB(total_local_hitbytes_) << " MiB" << std::endl;
        oss << "total cooperative hit bytes: " << B2MB(total_cooperative_hitbytes_) << " MiB" << std::endl;
        oss << "total request bytes: " << B2MB(total_reqbytes_) << " MiB" << std::endl;
        oss << "local byte hit ratio: " << getLocalByteHitRatio() << std::endl;
        oss << "cooperative byte hit ratio: " << getCooperativeByteHitRatio() << std::endl;
        oss << "total byte hit ratio: " << getTotalByteHitRatio() << std::endl;

        // NOTE: NO need weighted global byte hit ratio due to the same reasons mentioned above
        // oss << "total byte hit ratio (weighted by " << tmp_weight << "): " << getLocalByteHitRatio() + tmp_weight * getCooperativeByteHitRatio() << std::endl;

        oss << "[Latency Statistics]" << std::endl;
        oss << "average latency: " << avg_latency_ << std::endl;
        oss << "min latency: " << min_latency_ << std::endl;
        oss << "medium latency: " << medium_latency_ << std::endl;
        oss << "90th-percentile latency: " << tail90_latency_ << std::endl;
        oss << "95th-percentile latency: " << tail95_latency_ << std::endl;
        oss << "99th-percentile latency: " << tail99_latency_ << std::endl;
        oss << "max latency: " << max_latency_ << std::endl;

        oss << "[Read-write Ratio Statistics]" << std::endl;
        oss << "total read cnt: " << total_readcnt_ << std::endl;
        oss << "total write cnt: " << total_writecnt_ << std::endl;

        oss << "[Cache Utilization Statistics]" << std::endl;
        oss << "total cache size usage bytes: " << total_cache_size_bytes_ << std::endl;
        oss << "total cache capacity bytes: " << total_cache_capacity_bytes_ << std::endl;
        oss << "total cache margin bytes: " << getTotalCacheMarginBytes() << std::endl;
        oss << "total cache utilization: " << getTotalCacheUtilization() << std::endl;

        oss << "[Workload key-value Size Statistics]" << std::endl;
        oss << "total workload key size: " << B2MB(total_workload_key_size_) << " MiB" << std::endl;
        oss << "total workload value size: " << B2MB(total_workload_value_size_) << " MiB" << std::endl;
        oss << "average workload key size: " << getAvgWorkloadKeySize() << " bytes" << std::endl;
        oss << "average workload value size: " << B2KB(getAvgWorkloadValueSize()) << " KiB" << std::endl;

        oss << "[Bandwidth Usage Statistics]" << std::endl;
        oss << "total client-edge bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getClientEdgeBandwidthBytes())) << " MiB" << std::endl;
        oss << "total cross-edge bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getCrossEdgeBandwidthBytes())) << " MiB" << std::endl;
        oss << "total edge-cloud bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getEdgeCloudBandwidthBytes())) << " MiB" << std::endl;
        if (total_reqcnt_ > 0)
        {
            oss << "per-request client-edge bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getClientEdgeBandwidthBytes()) / static_cast<double>(total_reqcnt_)) << " MiB/req" << std::endl;
            oss << "per-request cross-edge bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getCrossEdgeBandwidthBytes()) / static_cast<double>(total_reqcnt_)) << " MiB/req" << std::endl;
            oss << "per-request edge-cloud bandwidth usage: " << B2MB(static_cast<double>(total_bandwidth_usage_.getEdgeCloudBandwidthBytes()) / static_cast<double>(total_reqcnt_)) << " MiB/req" << std::endl;
        }
        
        std::string total_statistics_string = oss.str();
        return total_statistics_string;
    }

    uint32_t AggregatedStatisticsBase::getAggregatedStatisticsIOSize()
    {
        // Aggregated statistics for object hit ratio + byte hit ratio + latency + read-write ratio + cache utilization + workload key-value size + bandwidth usgae
        return sizeof(uint32_t) * 3 + sizeof(double) * 3 + sizeof(uint32_t) * 7 + sizeof(uint32_t) * 2 + sizeof(uint64_t) * 2 + 2 * sizeof(double) + BandwidthUsage::getBandwidthUsagePayloadSize();
    }

    uint32_t AggregatedStatisticsBase::serialize(DynamicArray& dynamic_array, const uint32_t& position) const
    {
        uint32_t size = position;

        // Serialize aggregated statistics for object hit ratio
        dynamic_array.deserialize(size, (const char*)&total_local_hitcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&total_cooperative_hitcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&total_reqcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Serialize aggregated statistis for byte hit raito
        dynamic_array.deserialize(size, (const char*)&total_local_hitbytes_, sizeof(double));
        size += sizeof(double);
        dynamic_array.deserialize(size, (const char*)&total_cooperative_hitbytes_, sizeof(double));
        size += sizeof(double);
        dynamic_array.deserialize(size, (const char*)&total_reqbytes_, sizeof(double));
        size += sizeof(double);

        // Serialize aggregated statistics for latency
        dynamic_array.deserialize(size, (const char*)&avg_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&min_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&medium_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&tail90_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&tail95_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&tail99_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&max_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Serialize aggregated statistics for read-write ratio
        dynamic_array.deserialize(size, (const char*)&total_readcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.deserialize(size, (const char*)&total_writecnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Serialize aggregated statistics for cache utilization
        dynamic_array.deserialize(size, (const char*)&total_cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        dynamic_array.deserialize(size, (const char*)&total_cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);

        // Serialize aggregated statistics for workload key-value size
        dynamic_array.deserialize(size, (const char*)&total_workload_key_size_, sizeof(double));
        size += sizeof(double);
        dynamic_array.deserialize(size, (const char*)&total_workload_value_size_, sizeof(double));
        size += sizeof(double);

        // Serialize aggregated statistics for bandwidth usage
        uint32_t total_bandwidth_usage_serialize_size = total_bandwidth_usage_.serialize(dynamic_array, size);
        size += total_bandwidth_usage_serialize_size;

        return size - position;
    }

    uint32_t AggregatedStatisticsBase::deserialize(const DynamicArray& dynamic_array, const uint32_t& position)
    {
        uint32_t size = position;

        // Deserialize aggregated statistics for object hit ratio
        dynamic_array.serialize(size, (char *)&total_local_hitcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&total_cooperative_hitcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&total_reqcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Deserialize aggregated statistics for byte hit ratio
        dynamic_array.serialize(size, (char *)&total_local_hitbytes_, sizeof(double));
        size += sizeof(double);
        dynamic_array.serialize(size, (char *)&total_cooperative_hitbytes_, sizeof(double));
        size += sizeof(double);
        dynamic_array.serialize(size, (char *)&total_reqbytes_, sizeof(double));
        size += sizeof(double);

        // Deserialize aggregated statistics for latency
        dynamic_array.serialize(size, (char *)&avg_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&min_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&medium_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&tail90_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&tail95_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&tail99_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&max_latency_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Deserialize aggregated statistics for read-write ratio
        dynamic_array.serialize(size, (char *)&total_readcnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        dynamic_array.serialize(size, (char *)&total_writecnt_, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Deserialize aggregated statistics for cache utilization
        dynamic_array.serialize(size, (char*)&total_cache_size_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        dynamic_array.serialize(size, (char*)&total_cache_capacity_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);

        // Deserialize aggregated statistics for workload key-value size
        dynamic_array.serialize(size, (char*)&total_workload_key_size_, sizeof(double));
        size += sizeof(double);
        dynamic_array.serialize(size, (char*)&total_workload_value_size_, sizeof(double));
        size += sizeof(double);

        // Deserialize aggregated statistics for bandwidth usage
        uint32_t total_bandwidth_usage_deserialize_size = total_bandwidth_usage_.deserialize(dynamic_array, size);
        size += total_bandwidth_usage_deserialize_size;

        return size - position;
    }

    const AggregatedStatisticsBase& AggregatedStatisticsBase::operator=(const AggregatedStatisticsBase& other)
    {
        // Aggregated statistics related with object hit ratio
        total_local_hitcnt_ = other.total_local_hitcnt_;
        total_cooperative_hitcnt_ = other.total_cooperative_hitcnt_;
        total_reqcnt_ = other.total_reqcnt_;

        // Aggregated statistics related with byte hit ratio
        total_local_hitbytes_ = other.total_local_hitbytes_;
        total_cooperative_hitbytes_ = other.total_cooperative_hitbytes_;
        total_reqbytes_ = other.total_reqbytes_;

        // Aggregated statistics related with latency
        avg_latency_ = other.avg_latency_;
        min_latency_ = other.min_latency_;
        medium_latency_ = other.medium_latency_;
        tail90_latency_ = other.tail90_latency_;
        tail95_latency_ = other.tail95_latency_;
        tail99_latency_ = other.tail99_latency_;
        max_latency_ = other.max_latency_;

        // Aggregated statistics related with read-write ratio
        total_readcnt_ = other.total_readcnt_;
        total_writecnt_ = other.total_writecnt_;

        // Aggregated statistics related with cache utilization
        total_cache_size_bytes_ = other.total_cache_size_bytes_;
        total_cache_capacity_bytes_ = other.total_cache_capacity_bytes_;

        // Aggregated statistics related with workload key-value size
        total_workload_key_size_ = other.total_workload_key_size_;
        total_workload_value_size_ = other.total_workload_value_size_;

        // Aggregated statistics related with bandwidth usage
        total_bandwidth_usage_ = other.total_bandwidth_usage_;

        return *this;
    }
}