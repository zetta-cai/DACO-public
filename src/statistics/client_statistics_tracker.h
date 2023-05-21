/*
 * ClientStatisticsTracker: track and dump per-client statistics.
 * 
 * By Siyuan Sheng (2023.05.21).
 */

#ifndef CLIENT_STATISTICS_TRACKER_H
#define CLIENT_STATISTICS_TRACKER_H

#include <string>

namespace covered
{
    class ClientStatisticsTracker
    {
    public:
        ClientStatisticsTracker(uint32_t perclient_workercnt);
        ~ClientStatisticsTracker();

        // TODO: Add is_hit into local response messages (inherited from KeyHitflagMessage and KeyValueHitflagMessage)
        // TODO: Measure latency for each request
        // TODO: Update per-client statistics
    private:
        static const std::string kClassName;

        uint32_t* perworker_hitcnts_ptr_;
        uint32_t* perworker_reqcnts_ptr_;
        uint32_t* const latency_histogram_ptr_;

    };
}

#endif