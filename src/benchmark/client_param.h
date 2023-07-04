/*
 * ClientParam: parameters to launch a client for simulation (thread safe).
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_PARAM_H
#define CLIENT_PARAM_H

#include <atomic>
#include <string>

#include "common/node_param_base.h"
#include "statistics/client_statistics_tracker.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ClientParam : public NodeParamBase
    {
    public:
        ClientParam();
        ClientParam(const uint32_t& client_idx, WorkloadWrapperBase* workload_generator_ptr, ClientStatisticsTracker* client_statistics_tracker_ptr);
        ~ClientParam();

        const ClientParam& operator=(const ClientParam& other);

        WorkloadWrapperBase* getWorkloadGeneratorPtr();
        ClientStatisticsTracker* getClientStatisticsTrackerPtr();
    private:
        static const std::string kClassName;

        WorkloadWrapperBase* workload_generator_ptr_; // thread safe
        ClientStatisticsTracker* client_statistics_tracker_ptr_; // thread safe
    };
}

#endif