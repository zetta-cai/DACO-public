/*
 * ClientParam: parameters to launch a client for simulation.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_PARAM_H
#define CLIENT_PARAM_H

#include <atomic>
#include <string>

#include "statistics/client_statistics_tracker.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ClientParam
    {
    public:
        ClientParam();
        ClientParam(const uint32_t& client_idx, WorkloadWrapperBase* workload_generator_ptr, ClientStatisticsTracker* client_statistics_tracker_ptr);
        ~ClientParam();

        const ClientParam& operator=(const ClientParam& other);

        bool isClientRunning();
        void setClientRunning();
        void resetClientRunning();

        uint32_t getClientIdx();
        WorkloadWrapperBase* getWorkloadGeneratorPtr();
        ClientStatisticsTracker* getClientStatisticsTrackerPtr();
    private:
        static const std::string kClassName;

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // Cache coherence: MSI protocol.
        // Cache consistency: volatile.
        volatile std::atomic<bool> current_client_running_;

        uint32_t client_idx_;
        WorkloadWrapperBase* workload_generator_ptr_;
        ClientStatisticsTracker* client_statistics_tracker_ptr_;
    };
}

#endif