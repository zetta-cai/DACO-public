#include "benchmark/client_param.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : local_client_running_(false)
    {
        global_client_idx = 0;
        local_client_workload_startport_ = 0;
        local_edge_node_ipstr_ = "";
    }

    ClientParam::ClientParam(const uint32_t& global_client_idx, const uint16_t& local_client_workload_startport, const std::string& local_edge_node_ipstr) : local_client_running_(false)
    {
        global_client_idx_ = global_client_idx;
        local_client_workload_startport_ = local_client_workload_startport;
        local_edge_node_ipstr_ = local_edge_node_ipstr;
    }
        
    ClientParam::~ClientParam() {}

    const ClientParam& ClientParam::operator=(const ClientParam& other)
    {
        local_client_workload_startport_ = other.local_client_workload_startport_;
        local_edge_node_ipstr_ = other.local_edge_node_ipstr_;
        return *this;
    }

    uint32_t ClientParam::getGlobalClientIdx()
    {
        return global_client_idx_;
    }

    uint16_t ClientParam::getLocalClientWorkloadStartport()
    {
        return local_client_workload_startport_;
    }

    std::string ClientParam::getLocalEdgeNodeIpstr()
    {
        return local_edge_node_ipstr_;
    }
}