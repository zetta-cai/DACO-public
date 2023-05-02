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
        ClientParam(const uint32_t& global_client_idx, const std::string& local_edge_node_ipstr, WorkloadBase* workload_generator_ptr);
        ~ClientParam();

        const ClientParam& operator=(const ClientParam& other);

        bool isClientRunning();
        void setClientRunning();
        void resetClientRunning();

        uint32_t getGlobalClientIdx();
        std::string getLocalEdgeNodeIpstr();
        WorkloadBase* getWorkloadGeneratorPtr();
    private:
        static const std::string kClassName;

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // Cache coherence: MSI protocol.
        // Cache consistency: volatile.
        volatile std::atomic<bool> local_client_running_;

        uint32_t global_client_idx_;
        // Per-client UDP port range is [local_client_workload_startport, local_client_workload_startport + perclient_workercnt - 1]
        std::string local_edge_node_ipstr_;
        WorkloadBase* workload_generator_ptr_;
    };
}

#endif