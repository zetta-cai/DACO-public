#include "statistics/client_updated_statistics.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientUpdatedStatistics::kClassName("ClientUpdatedStatistics");

    ClientUpdatedStatistics::ClientUpdatedStatistics(uint32_t perclient_workercnt, const uint32_t& client_idx)
    {
        // Differentiate ClientStatisticsWrapper threads
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        perclient_workercnt_ = perclient_workercnt;
        latency_histogram_size_ = Config::getLatencyHistogramSize();

        perclientworker_local_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_local_hitcnts_ != NULL);
        Util::initializeAtomicArray(perclientworker_local_hitcnts_, perclient_workercnt, 0);

        perclientworker_cooperative_hitcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        Util::initializeAtomicArray(perclientworker_cooperative_hitcnts_, perclient_workercnt, 0);

        perclientworker_reqcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_reqcnts_ != NULL);
        Util::initializeAtomicArray(perclientworker_reqcnts_, perclient_workercnt, 0);

        latency_histogram_ = new std::atomic<uint32_t>[latency_histogram_size_];
        assert(latency_histogram_ != NULL);
        Util::initializeAtomicArray(latency_histogram_, latency_histogram_size_, 0);

        perclientworker_readcnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_readcnts_ != NULL);
        Util::initializeAtomicArray(perclientworker_readcnts_, perclient_workercnt, 0);

        perclientworker_writecnts_ = new std::atomic<uint32_t>[perclient_workercnt];
        assert(perclientworker_writecnts_ != NULL);
        Util::initializeAtomicArray(perclientworker_writecnts_, perclient_workercnt, 0);
    }

    ClientUpdatedStatistics::~ClientUpdatedStatistics()
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

    void ClientUpdatedStatistics::updateLocalHitcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_local_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_local_hitcnts_[local_client_worker_idx]++;
        updateReqcnt(local_client_worker_idx);
        return;
    }

    void ClientUpdatedStatistics::updateCooperativeHitcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_cooperative_hitcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_cooperative_hitcnts_[local_client_worker_idx]++;
        updateReqcnt(local_client_worker_idx);
        return;
    }

    void ClientUpdatedStatistics::updateReqcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_reqcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_reqcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientUpdatedStatistics::updateLatency(const uint32_t& latency_us)
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

    void ClientUpdatedStatistics::updateReadcnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_readcnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_readcnts_[local_client_worker_idx]++;
        return;
    }

    void ClientUpdatedStatistics::updateWritecnt(const uint32_t& local_client_worker_idx)
    {
        assert(perclientworker_writecnts_ != NULL);
        assert(local_client_worker_idx < perclient_workercnt_);

        perclientworker_writecnts_[local_client_worker_idx]++;
        return;
    }
}