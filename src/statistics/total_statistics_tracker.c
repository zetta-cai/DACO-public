#include "statistics/total_statistics_tracker.h"

#include <cstring> // memset

#include "common/util.h"

namespace covered
{
    const std::string kClassName("TotalStatisticsTracker");
    
    TotalStatisticsTracker::TotalStatisticsTracker(uint32_t clientcnt, ClientStatisticsTracker* client_statistics_trackers)
    {
        // Aggregate per-client hit ratio and latency statistics
        aggregateClientStatistics_(clientcnt, client_statistics_trackers);

        // Process aggregate statistics
        processAggregateStatistics_();
    }
        
    TotalStatisticsTracker::~TotalStatisticsTracker()
    {
        // Release space for aggregate statistics
        assert(perclient_local_hitcnts_ != NULL);
        delete perclient_local_hitcnts_;
        perclient_local_hitcnts_ = NULL;
        assert(perclient_cooperative_hitcnts_ != NULL);
        delete perclient_cooperative_hitcnts_;
        perclient_cooperative_hitcnts_ = NULL;
        assert(perclient_reqcnts_ != NULL);
        delete perclient_reqcnts_;
        perclient_reqcnts_ = NULL;
        assert(latency_histogram_ != NULL);
        delete latency_histogram_;
        latency_histogram_ = NULL;
    }

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
    
    void TotalStatisticsTracker::aggregateClientStatistics_(uint32_t clientcnt, ClientStatisticsTracker* client_statistics_trackers)
    {
        clientcnt_ = clientcnt;

        assert(client_statistics_trackers != NULL);
        uint32_t perclient_workercnt = client_statistics_trackers[0].getPerclientWorkercnt();
        latency_histogram_size_ = client_statistics_trackers[0].getLatencyHistogramSize();

        // Allocate space for aggregate statistics
        perclient_local_hitcnts_ = new uint32_t[clientcnt_];
        assert(perclient_local_hitcnts_ != NULL);
        memset(perclient_local_hitcnts_, 0, clientcnt_ * sizeof(uint32_t));
        perclient_cooperative_hitcnts_ = new uint32_t[clientcnt_];
        assert(perclient_cooperative_hitcnts_ != NULL);
        memset(perclient_cooperative_hitcnts_, 0, clientcnt_ * sizeof(uint32_t));
        perclient_reqcnts_ = new uint32_t[clientcnt_];
        assert(perclient_reqcnts_ != NULL);
        memset(perclient_reqcnts_, 0, clientcnt_ * sizeof(uint32_t));
        latency_histogram_ = new uint32_t[latency_histogram_size_];
        assert(latency_histogram_ != NULL);
        memset(latency_histogram_, 0, latency_histogram_size_ * sizeof(uint32_t));

        // Aggregate per-client statistics
        for (uint32_t global_client_idx = 0; global_client_idx < clientcnt_; i++)
        {
            const ClientStatisticsTracker& tmp_client_statistics_tracker = client_statistics_trackers[global_client_idx];
            assert(perclient_workercnt == tmp_client_statistics_tracker.getPerclientWorkercnt());
            assert(latency_histogram_size_ == tmp_client_statistics_tracker.getLatencyHistogram());

            // Aggregate per-client hit ratio statistics
            std::atomic<uint32_t>* tmp_perworker_local_hitcnts = tmp_client_statistics_tracker.getPerworkerLocalHitcnts();
            std::atomic<uint32_t>* tmp_perworker_cooperative_hitcnts = tmp_client_statistics_tracker.getPerworkerCooperativeHitcnts();
            std::atomic<uint32_t>* tmp_perworker_reqcnts = tmp_client_statistics_tracker.getPerworkerReqcnts();
            for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
            {
                perclient_local_hitcnts_[global_client_idx] += tmp_perworker_local_hitcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
                perclient_cooperative_hitcnts_[global_client_idx] += tmp_perworker_cooperative_hitcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
                perclient_reqcnts_[global_client_idx] += tmp_perworker_reqcnts[local_worker_idx].load(Util::LOAD_CONCURRENCY_ORDER);
            }

            // Aggregate per-client latency statistics
            std::atomic<uint32_t>* tmp_latency_histogram = tmp_client_statistics_tracker.getLatencyHistogram();
            for (uint32_t latency_us = 0; latency_us < latency_histogram_size_; latency_us++)
            {
                latency_histogram_[latency_us] += tmp_latency_histogram[latency_us];
            }
        }

        return;
    }

    void TotalStatisticsTracker::processAggregateStatistics_()
    {
        assert(perclient_local_hitcnts_ != NULL);
        total_local_hitcnt_ = 0;
        for (uint32_t global_client_idx = 0; global_client_idx < clientcnt_; global_client_idx++)
        {
            total_local_hitcnt_ += perclient_local_hitcnts_[global_client_idx];
        }

        assert(perclient_cooperative_hitcnts_ != NULL);
        total_cooperative_hitcnt_ = 0;
        for (uint32_t global_client_idx = 0; global_client_idx < clientcnt_; global_client_idx++)
        {
            total_cooperative_hitcnt_ += perclient_cooperative_hitcnts_[global_client_idx];
        }

        assert(perclient_reqcnts_ != NULL);
        total_reqcnt_ = 0;
        for (uint32_t global_client_idx = 0; global_client_idx < clientcnt_; global_client_idx++)
        {
            total_reqcnt_ += perclient_reqcnts_[global_client_idx];
        }

        assert(latency_histogram_ != NULL);
        // Get latency overview to prepare for latency statistics
        total_latency_cnt = 0; // i.e., total reqcnt
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
            double cur_ratio = static_cast<double>(cur_latency_cnt) / static_cast<double>(total_latency_cnt);

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
        tmp_avg_latency /= total_latency_cnt;
        assert(tmp_avg_latency >= 0 && tmp_avg_latency < latency_histogram_size_);
        avg_latency_ = static_cast<uint32_t>(tmp_avg_latency);

        return;
    }
}