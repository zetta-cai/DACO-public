#include "benchmark/client_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : local_client_running_(false)
    {
        global_client_idx_ = 0;
        local_edge_node_ipstr_ = "";
        workload_generator_ptr_ = NULL;
    }

    ClientParam::ClientParam(const uint32_t& global_client_idx, const std::string& local_edge_node_ipstr, WorkloadBase* workload_generator_ptr) : local_client_running_(false)
    {
        global_client_idx_ = global_client_idx;
        local_edge_node_ipstr_ = local_edge_node_ipstr;
        if (workload_generator_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "workload_generator_ptr is NULL!");
            exit(1);
        }
        workload_generator_ptr_ = workload_generator_ptr;
    }
        
    ClientParam::~ClientParam() {}

    const ClientParam& ClientParam::operator=(const ClientParam& other)
    {
        local_client_running_.store(other.local_client_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        global_client_idx_ = other.global_client_idx_;
        local_edge_node_ipstr_ = other.local_edge_node_ipstr_;
        if (other.workload_generator_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.workload_generator_ptr_ is NULL!");
            exit(1);
        }
        workload_generator_ptr_ = other.workload_generator_ptr_;
        return *this;
    }

    bool ClientParam::isClientRunning()
    {
        return local_client_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void ClientParam::setClientRunning()
    {
        return local_client_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void ClientParam::resetClientRunning()
    {
        return local_client_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    uint32_t ClientParam::getGlobalClientIdx()
    {
        return global_client_idx_;
    }

    std::string ClientParam::getLocalEdgeNodeIpstr()
    {
        return local_edge_node_ipstr_;
    }

    WorkloadBase* ClientParam::getWorkloadGeneratorPtr()
    {
        return workload_generator_ptr_;
    }
}