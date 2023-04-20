#include "benchmark/physical_client_param.h"

namespace covered
{
    const std::string PhysicalClientParam::kClassName = "PhysicalClientParam";

    PhysicalClientParam::PhysicalClientParam(const uint16_t& local_client_workload_startport, const std::string& local_edge_node_ipstr)
    {
        local_client_workload_startport_ = local_client_workload_startport;
        local_edge_node_ipstr_ = local_edge_node_ipstr;
    }
        
    PhysicalClientParam::~PhysicalClientParam() {}
}