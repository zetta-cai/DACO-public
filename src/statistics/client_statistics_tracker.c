#include "statistics/client_statistics_tracker.h"

#include <cstring> // memcpy memset

namespace covered
{
    const std::string kClassName("ClientStatisticsTracker");
    
    ClientStatisticsTracker::ClientStatisticsTracker(uint32_t perclient_workercnt) : latency_histogram_ptr(new uint32_t[Config::getLatencyHistogramSize()])
    {
        assert(latency_histogram_ptr_ != NULL);
        memset((void*)latency_histogram_ptr_, 0, Config::getLatencyHistogramSize() * sizeof(uint32_t));

        perworker_hitcnts_ptr_ = new uint32_t[perclient_workercnt];
        assert(perworker_hitcnts_ptr_ != NULL);
        memset((void*)perworker_hitcnts_ptr_, 0, perclient_workercnt * sizeof(uint32_t));

        perworker_reqcnts_ptr_ = new uint32_t[perclient_workercnt];;
        assert(perworker_reqcnts_ptr_ != NULL);
        memset((void*)perworker_reqcnts_ptr_, 0, perclient_workercnt * sizeof(uint32_t));
    }

    ClientStatisticsTracker::~ClientStatisticsTracker()
    {
        // Free latency histogram
        assert(latency_histogram_ptr_ != NULL);
        delete latency_histogram_ptr_;
        //latency_histogram_ptr_ = NULL;
    }
}