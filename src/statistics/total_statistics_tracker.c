#include "statistics/total_statistics_tracker.h"

#include <assert.h>
//#include <cstring> // memset
#include <sstream>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string TotalStatisticsTracker::kClassName("TotalStatisticsTracker");
    
    TotalStatisticsTracker::TotalStatisticsTracker(uint32_t clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs)
    {
        clientcnt_ = clientcnt;
        latency_histogram_size_ = Config::getLatencyHistogramSize();

        // Pre-process per-client hit ratio and latency statistics
        preprocessClientStatistics_(clientcnt, client_statistics_tracker_ptrs);

        // Generate aggregate statistics
        aggregateClientStatistics_();
    }
        
    TotalStatisticsTracker::~TotalStatisticsTracker() {}

    uint32_t TotalStatisticsTracker::getTotalLocalHitcnt() const
    {
        return total_local_hitcnt_;
    }
    
    uint32_t TotalStatisticsTracker::getTotalCooperativeHitcnt() const
    {
        return total_cooperative_hitcnt_;
    }
    
    uint32_t TotalStatisticsTracker::getTotalReqcnt() const
    {
        return total_reqcnt_;
    }

    uint32_t TotalStatisticsTracker::getAvgLatency() const
    {
        return avg_latency_;
    }
    
    uint32_t TotalStatisticsTracker::getMinLatency() const
    {
        return min_latency_;
    }
    
    uint32_t TotalStatisticsTracker::getMediumLatency() const
    {
        return medium_latency_;
    }

    uint32_t TotalStatisticsTracker::getTail90Latency() const
    {
        return tail90_latency_;
    }

    uint32_t TotalStatisticsTracker::getTail95Latency() const
    {
        return tail90_latency_;
    }

    uint32_t TotalStatisticsTracker::getTail99Latency() const
    {
        return tail90_latency_;
    }
    
    uint32_t TotalStatisticsTracker::getMaxLatency() const
    {
        return max_latency_;
    }

    uint32_t TotalStatisticsTracker::getTotalReadcnt() const
    {
        return total_readcnt_;
    }

    uint32_t TotalStatisticsTracker::getTotalWritecnt() const
    {
        return total_writecnt_;
    }

    std::string TotalStatisticsTracker::toString() const
    {
        std::ostringstream oss;
        oss << "[Hit Ratio Statistics]" << std::endl;
        oss << "total local hit cnts: " << total_local_hitcnt_ << std::endl;
        oss << "total cooperative hit cnts: " << total_cooperative_hitcnt_ << std::endl;
        oss << "total request cnts: " << total_reqcnt_ << std::endl;
        oss << "[Latency Statistics]" << std::endl;
        oss << "average latency: " << avg_latency_ << std::endl;
        oss << "min latency: " << min_latency_ << std::endl;
        oss << "medium latency: " << medium_latency_ << std::endl;
        oss << "90th-percentile latency: " << tail90_latency_ << std::endl;
        oss << "95th-percentile latency: " << tail95_latency_ << std::endl;
        oss << "99th-percentile latency: " << tail99_latency_ << std::endl;
        oss << "max latency: " << max_latency_ << std::endl;
        oss << "total read cnt: " << total_readcnt_ << std::endl;
        oss << "total write cnt: " << total_writecnt_;
        
        std::string total_statistics_string = oss.str();
        return total_statistics_string;
    }
    
    void TotalStatisticsTracker::preprocessClientStatistics_(uint32_t clientcnt, ClientStatisticsTracker** client_statistics_tracker_ptrs)
    {
        assert(client_statistics_tracker_ptrs != NULL);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            assert(client_statistics_tracker_ptrs[client_idx] != NULL);
        }

        uint32_t perclient_workercnt = client_statistics_tracker_ptrs[0]->getPerclientWorkercnt();
        //latency_histogram_size_ = client_statistics_tracker_ptrs[0]->getLatencyHistogramSize();

        // Allocate space for aggregate statistics
        perclient_local_hitcnts_.resize(clientcnt_, 0);
        perclient_cooperative_hitcnts_.resize(clientcnt_, 0);
        perclient_reqcnts_.resize(clientcnt_, 0);
        latency_histogram_.resize(latency_histogram_size_, 0);
        perclient_readcnts_.resize(clientcnt_, 0);
        perclient_writecnts_.resize(clientcnt_, 0);

        // Aggregate per-client statistics
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            const ClientStatisticsTracker& tmp_client_statistics_tracker = *(client_statistics_tracker_ptrs[client_idx]);
            assert(perclient_workercnt == tmp_client_statistics_tracker.getPerclientWorkercnt());
            assert(latency_histogram_size_ == tmp_client_statistics_tracker.getLatencyHistogramSize());

            // Aggregate per-client hit ratio statistics
            std::atomic<uint32_t>* tmp_perclientworker_local_hitcnts = tmp_client_statistics_tracker.getPerclientworkerLocalHitcnts();
            assert(tmp_perclientworker_local_hitcnts != NULL);
            std::atomic<uint32_t>* tmp_perclientworker_cooperative_hitcnts = tmp_client_statistics_tracker.getPerclientworkerCooperativeHitcnts();
            assert(tmp_perclientworker_cooperative_hitcnts != NULL);
            std::atomic<uint32_t>* tmp_perclientworker_reqcnts = tmp_client_statistics_tracker.getPerclientworkerReqcnts();
            assert(tmp_perclientworker_reqcnts != NULL);
            for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
            {
                perclient_local_hitcnts_[client_idx] += tmp_perclientworker_local_hitcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
                perclient_cooperative_hitcnts_[client_idx] += tmp_perclientworker_cooperative_hitcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
                perclient_reqcnts_[client_idx] += tmp_perclientworker_reqcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            }

            // Aggregate per-client latency statistics
            std::atomic<uint32_t>* tmp_latency_histogram = tmp_client_statistics_tracker.getLatencyHistogram();
            assert(tmp_latency_histogram != NULL);
            for (uint32_t latency_us = 0; latency_us < latency_histogram_size_; latency_us++)
            {
                latency_histogram_[latency_us] += tmp_latency_histogram[latency_us];
            }

            // Aggregate per-client read-write statistics
            std::atomic<uint32_t>* tmp_perclientworker_readcnts = tmp_client_statistics_tracker.getPerclientworkerReadcnts();
            assert(tmp_perclientworker_readcnts != NULL);
            std::atomic<uint32_t>* tmp_perclientworker_writecnts = tmp_client_statistics_tracker.getPerclientworkerWritecnts();
            assert(tmp_perclientworker_writecnts != NULL);
            for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
            {
                perclient_readcnts_[client_idx] += tmp_perclientworker_readcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
                perclient_writecnts_[client_idx] += tmp_perclientworker_writecnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            }
        }

        return;
    }

    void TotalStatisticsTracker::aggregateClientStatistics_()
    {
        assert(perclient_local_hitcnts_.size() == clientcnt_);
        total_local_hitcnt_ = 0;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            total_local_hitcnt_ += perclient_local_hitcnts_[client_idx];
        }

        assert(perclient_cooperative_hitcnts_.size() == clientcnt_);
        total_cooperative_hitcnt_ = 0;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            total_cooperative_hitcnt_ += perclient_cooperative_hitcnts_[client_idx];
        }

        assert(perclient_reqcnts_.size() == clientcnt_);
        total_reqcnt_ = 0;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            total_reqcnt_ += perclient_reqcnts_[client_idx];
        }

        assert(latency_histogram_.size() == latency_histogram_size_);
        // Get latency overview to prepare for latency statistics
        uint32_t total_latency_cnt = 0; // i.e., total reqcnt
        for (uint32_t latency_us = 0; latency_us < latency_histogram_size_; latency_us++)
        {
            uint32_t tmp_latency_cnt = latency_histogram_[latency_us];
            total_latency_cnt += tmp_latency_cnt;
        }
        // Get latency statistics
        uint32_t cur_latency_cnt = 0;
        uint64_t tmp_avg_latency = 0; // To avoid integer overflow
        avg_latency_ = 0;
        min_latency_ = 0;
        medium_latency_ = 0;
        tail90_latency_ = 0;
        tail95_latency_ = 0;
        tail99_latency_ = 0;
        max_latency_ = 0;
        for (uint32_t latency_us = 0; latency_us < latency_histogram_size_; latency_us++)
        {
            uint32_t tmp_latency_cnt = latency_histogram_[latency_us];
            cur_latency_cnt += tmp_latency_cnt;
            double cur_ratio = 0.0d;
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
        assert(tmp_avg_latency >= 0 && tmp_avg_latency < latency_histogram_size_);
        avg_latency_ = static_cast<uint32_t>(tmp_avg_latency);

        assert(perclient_readcnts_.size() == clientcnt_);
        total_readcnt_ = 0;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            total_readcnt_ += perclient_readcnts_[client_idx];
        }

        assert(perclient_writecnts_.size() == clientcnt_);
        total_writecnt_ = 0;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            total_writecnt_ += perclient_writecnts_[client_idx];
        }

        return;
    }
}