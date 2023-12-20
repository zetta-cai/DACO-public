#include "statistics/client_raw_statistics.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientRawStatistics::kClassName("ClientRawStatistics");

    ClientRawStatistics::ClientRawStatistics(uint32_t perclient_workercnt)
    {
        perclient_workercnt_ = perclient_workercnt;
        latency_histogram_size_ = Config::getLatencyHistogramSize();

        perclientworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_local_hitcnts_ != NULL);
        Util::initializeAtomicArray<uint32_t>(perclientworker_local_hitcnts_, perclient_workercnt, 0);

        perclientworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        Util::initializeAtomicArray<uint32_t>(perclientworker_cooperative_hitcnts_, perclient_workercnt, 0);

        perclientworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_reqcnts_ != NULL);
        Util::initializeAtomicArray<uint32_t>(perclientworker_reqcnts_, perclient_workercnt, 0);

        perclientworker_local_hitbytes_.resize(perclient_workercnt, double(0.0));
        perclientworker_cooperative_hitbytes_.resize(perclient_workercnt, double(0.0));
        perclientworker_reqbytes_.resize(perclient_workercnt, double(0.0));

        latency_histogram_ = new std::atomic<uint32_t>[latency_histogram_size_];
        assert(latency_histogram_ != NULL);
        Util::initializeAtomicArray<uint32_t>(latency_histogram_, latency_histogram_size_, 0);

        perclientworker_readcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_readcnts_ != NULL);
        Util::initializeAtomicArray<uint32_t>(perclientworker_readcnts_, perclient_workercnt, 0);

        perclientworker_writecnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_writecnts_ != NULL);
        Util::initializeAtomicArray<uint32_t>(perclientworker_writecnts_, perclient_workercnt, 0);

        closest_edge_cache_size_bytes_ = 0;
        closest_edge_cache_capacity_bytes_ = 0;

        perclientworker_total_workload_key_sizes_.resize(perclient_workercnt, double(0.0));
        perclientworker_total_workload_value_sizes_.resize(perclient_workercnt, double(0.0));

        perclientworker_bandwidth_usages_.resize(perclient_workercnt, BandwidthUsage());
    }

    ClientRawStatistics::~ClientRawStatistics()
    {
        assert(perclientworker_local_hitcnts_ != NULL);
        delete[] perclientworker_local_hitcnts_;
        perclientworker_local_hitcnts_ = NULL;

        assert(perclientworker_cooperative_hitcnts_ != NULL);
        delete[] perclientworker_cooperative_hitcnts_;
        perclientworker_cooperative_hitcnts_ = NULL;
        
        assert(perclientworker_reqcnts_ != NULL);
        delete[] perclientworker_reqcnts_;
        perclientworker_reqcnts_ = NULL;

        assert(latency_histogram_ != NULL);
        delete[] latency_histogram_;
        latency_histogram_ = NULL;

        assert(perclientworker_readcnts_ != NULL);
        delete[] perclientworker_readcnts_;
        perclientworker_readcnts_ = NULL;

        assert(perclientworker_writecnts_ != NULL);
        delete[] perclientworker_writecnts_;
        perclientworker_writecnts_ = NULL;
    }

    // Get client raw statistics for debug
    uint32_t ClientRawStatistics::getReqcnt(const uint32_t& local_client_worker_idx) const
    {
        assert(perclientworker_reqcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        return perclientworker_reqcnts_[local_client_worker_idx];
    }

    void ClientRawStatistics::clean()
    {
        Util::initializeAtomicArray<uint32_t>(perclientworker_local_hitcnts_, perclient_workercnt_, 0);
        Util::initializeAtomicArray<uint32_t>(perclientworker_cooperative_hitcnts_, perclient_workercnt_, 0);
        Util::initializeAtomicArray<uint32_t>(perclientworker_reqcnts_, perclient_workercnt_, 0);

        perclientworker_local_hitbytes_.clear();
        perclientworker_local_hitbytes_.resize(perclient_workercnt_, double(0.0));
        perclientworker_cooperative_hitbytes_.clear();
        perclientworker_cooperative_hitbytes_.resize(perclient_workercnt_, double(0.0));
        perclientworker_reqbytes_.clear();
        perclientworker_reqbytes_.resize(perclient_workercnt_, double(0.0));

        Util::initializeAtomicArray<uint32_t>(latency_histogram_, latency_histogram_size_, 0);

        Util::initializeAtomicArray<uint32_t>(perclientworker_readcnts_, perclient_workercnt_, 0);
        Util::initializeAtomicArray<uint32_t>(perclientworker_writecnts_, perclient_workercnt_, 0);

        closest_edge_cache_size_bytes_ = 0;
        closest_edge_cache_capacity_bytes_ = 0;

        perclientworker_total_workload_key_sizes_.clear();
        perclientworker_total_workload_key_sizes_.resize(perclient_workercnt_, double(0.0));

        perclientworker_total_workload_value_sizes_.clear();
        perclientworker_total_workload_value_sizes_.resize(perclient_workercnt_, double(0.0));

        perclientworker_bandwidth_usages_.clear();
        perclientworker_bandwidth_usages_.resize(perclient_workercnt_, BandwidthUsage());

        return;
    }

    void ClientRawStatistics::updateLocalHitcnt_(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_local_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_local_hitcnts_[local_client_worker_idx]++;
        updateReqcnt_(local_client_worker_idx);
        return;
    }

    void ClientRawStatistics::updateCooperativeHitcnt_(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_cooperative_hitcnts_[local_client_worker_idx]++;
        updateReqcnt_(local_client_worker_idx);
        return;
    }

    void ClientRawStatistics::updateReqcnt_(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_reqcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_reqcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientRawStatistics::updateLocalHitbytes_(const uint32_t& local_client_worker_idx, const uint32_t& object_size)
    {
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_local_hitbytes_[local_client_worker_idx] += static_cast<double>(object_size);
        updateReqbytes_(local_client_worker_idx, object_size);
        return;
    }

    void ClientRawStatistics::updateCooperativeHitbytes_(const uint32_t& local_client_worker_idx, const uint32_t& object_size)
    {
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_cooperative_hitbytes_[local_client_worker_idx] += static_cast<double>(object_size);
        updateReqbytes_(local_client_worker_idx, object_size);
        return;
    }

    void ClientRawStatistics::updateReqbytes_(const uint32_t& local_client_worker_idx, const uint32_t& object_size)
    {
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_reqbytes_[local_client_worker_idx] += static_cast<double>(object_size);
        return;
    }

    uint32_t ClientRawStatistics::getMaxlatency_() const
    {
        assert(latency_histogram_ != NULL);

        for (uint32_t latency_us = latency_histogram_size_ - 1; latency_us >= 0; latency_us--)
        {
            if (latency_histogram_[latency_us] > 0)
            {
                return latency_us;
            }
        }

        return 0;
    }

    void ClientRawStatistics::updateLatency_(const uint32_t& latency_us)
    {
        assert(latency_histogram_ != NULL);

        if (latency_us < latency_histogram_size_)
        {
            latency_histogram_[latency_us]++;
        }
        else
        {
            latency_histogram_[latency_histogram_size_ - 1]++;
        }
        return;
    }

    void ClientRawStatistics::updateReadcnt_(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_readcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_readcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientRawStatistics::updateWritecnt_(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_writecnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_writecnts_[local_client_worker_idx]++;
        return;
    }

    void ClientRawStatistics::updateCacheUtilization_(const uint64_t& closest_edge_cache_size_bytes, const uint64_t& closest_edge_cache_capacity_bytes)
    {
        closest_edge_cache_size_bytes_ = closest_edge_cache_size_bytes;
        closest_edge_cache_capacity_bytes_ = closest_edge_cache_capacity_bytes;
        return;
    }

    void ClientRawStatistics::updateTotalWorkloadKeyValueSize_(const uint32_t& local_client_worker_idx, const uint32_t& key_size, const uint32_t& value_size)
    {
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_total_workload_key_sizes_[local_client_worker_idx] += static_cast<double>(key_size);
        perclientworker_total_workload_value_sizes_[local_client_worker_idx] += static_cast<double>(value_size);
        return;
    }

    void ClientRawStatistics::updateBandwidthUsage_(const uint32_t& local_client_worker_idx, const BandwidthUsage& bandwidth_usage)
    {
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_bandwidth_usages_[local_client_worker_idx].update(bandwidth_usage);
        return;
    }

    void ClientRawStatistics::checkPointers_() const
    {
        // Per-client-worker hit ratio statistics
        assert(perclientworker_local_hitcnts_ != NULL);
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        assert(perclientworker_reqcnts_ != NULL);

        // Per-client-worker latency statistics
        assert(latency_histogram_ != NULL);

        // Per-client-worker read-write ratio statistics
        assert(perclientworker_readcnts_ != NULL);
        assert(perclientworker_writecnts_ != NULL);
    }
}