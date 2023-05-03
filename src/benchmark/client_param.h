/*
 * ClientParam: parameters to launch a client for simulation.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_PARAM_H
#define CLIENT_PARAM_H

#include <atomic>
#include <string>

#include "workload/workload_base.h"

namespace covered
{
    class ClientParam
    {
    public:
        ClientParam();
        ClientParam(const uint32_t& global_client_idx, WorkloadBase* workload_generator_ptr);
        ~ClientParam();

        const ClientParam& operator=(const ClientParam& other);

        bool isClientRunning();
        void setClientRunning();
        void resetClientRunning();

        uint32_t getGlobalClientIdx();
        WorkloadBase* getWorkloadGeneratorPtr();
    private:
        static const std::string kClassName;

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // Cache coherence: MSI protocol.
        // Cache consistency: volatile.
        volatile std::atomic<bool> local_client_running_;

        uint32_t global_client_idx_;
        WorkloadBase* workload_generator_ptr_;
    };
}

#endif