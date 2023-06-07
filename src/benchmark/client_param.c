#include "benchmark/client_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : client_running_(false)
    {
        client_idx_ = 0;
        workload_generator_ptr_ = NULL;
        client_statistics_tracker_ptr_ = NULL;
    }

    ClientParam::ClientParam(const uint32_t& client_idx, WorkloadWrapperBase* workload_generator_ptr, ClientStatisticsTracker* client_statistics_tracker_ptr) : client_running_(false)
    {
        client_idx_ = client_idx;
        if (workload_generator_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "workload_generator_ptr is NULL!");
            exit(1);
        }
        workload_generator_ptr_ = workload_generator_ptr;
        if (client_statistics_tracker_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_statistics_tracker_ptr is NULL!");
            exit(1);
        }
        client_statistics_tracker_ptr_ = client_statistics_tracker_ptr;
    }
        
    ClientParam::~ClientParam()
    {
        // NOTE: no need to delete workload_generator_ptr_, as it is maintained outside ClientParam
        // NOTE: no need to delete client_statistics_tracker_ptr_, as it is maintained outside ClientParam
    }

    const ClientParam& ClientParam::operator=(const ClientParam& other)
    {
        client_running_.store(other.client_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        client_idx_ = other.client_idx_;
        if (other.workload_generator_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.workload_generator_ptr_ is NULL!");
            exit(1);
        }
        workload_generator_ptr_ = other.workload_generator_ptr_;
        if (other.client_statistics_tracker_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.client_statistics_tracker_ptr_ is NULL!");
            exit(1);
        }
        client_statistics_tracker_ptr_ = other.client_statistics_tracker_ptr_;
        return *this;
    }

    bool ClientParam::isClientRunning()
    {
        return client_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void ClientParam::setClientRunning()
    {
        return client_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void ClientParam::resetClientRunning()
    {
        return client_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    uint32_t ClientParam::getClientIdx()
    {
        return client_idx_;
    }

    WorkloadWrapperBase* ClientParam::getWorkloadGeneratorPtr()
    {
        return workload_generator_ptr_;
    }

    ClientStatisticsTracker* ClientParam::getClientStatisticsTrackerPtr()
    {
        return client_statistics_tracker_ptr_;
    }
}