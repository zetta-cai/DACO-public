#include "benchmark/physical_client.h"

namespace covered
{
    const std::string PhysicalClient::kClassName = "PhysicalClient";

    PhysicalClient::PhysicalClient(const uint16_t& local_client_workload_startport, const std::string& local_edge_node_ipstr, const uint32_t& client_thread_cnt)
    {
        client_workload_port_ = client_workload_port;
        local_edge_node_ipstr_ = local_edge_node_ipstr;
        client_thread_cnt_ = client_thread_cnt;
    }

    PhysicalClient::~PhysicalClient() {}

    void PhysicalClient::PhysicalClient(const uint32_t& duration)
    {
        // TODO: launch client_thread_cnt sub-threads

            // TODO: for each sub-thread, within a while loop for a duration

                // TODO: generate key-value request based on a specific workload

            // TODO: at the end of duration, notify main thread by pthread_exit
        
        // TODO: main thread dumps statistics every 10 seconds if necessary

        // TODO: main thread aggregates statistics after joining all sub-threads
    }
}