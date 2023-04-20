/*
 * PhysicalClientParam: parameters to launch a physical client for simulation.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef PHYSICAL_CLIENT_PARAM_H
#define PHYSICAL_CLIENT_PARAM_H

#include <string>

namespace covered
{
    class PhysicalClientParam
    {
    public:
        PhysicalClientParam(const uint16_t& local_client_workload_startport, const std::string& local_edge_node_ipstr);
        ~PhysicalClientParam();
    private:
        static const std::string kClassName;

        uint16_t local_client_workload_startport_;
        std::string local_edge_node_ipstr_;
    };
}

#endif