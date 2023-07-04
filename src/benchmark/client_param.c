#include "benchmark/client_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : NodeParamBase(0, false)
    {
        workload_generator_ptr_ = NULL;
        client_statistics_tracker_ptr_ = NULL;
    }

    ClientParam::ClientParam(const uint32_t& client_idx, WorkloadWrapperBase* workload_generator_ptr, ClientStatisticsTracker* client_statistics_tracker_ptr) : NodeParamBase(client_idx, false)
    {
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
        NodeParamBase::operator=(other);
        
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

    WorkloadWrapperBase* ClientParam::getWorkloadGeneratorPtr()
    {
        return workload_generator_ptr_;
    }

    ClientStatisticsTracker* ClientParam::getClientStatisticsTrackerPtr()
    {
        return client_statistics_tracker_ptr_;
    }
}