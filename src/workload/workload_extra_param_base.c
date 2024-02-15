#include "workload/workload_extra_param_base.h"

namespace covered
{
    const std::string WorkloadExtraParamBase::kClassName("WorkloadExtraParamBase");

    WorkloadExtraParamBase::WorkloadExtraParamBase()
    {
    }

    WorkloadExtraParamBase::~WorkloadExtraParamBase()
    {
    }

    const WorkloadExtraParamBase& WorkloadExtraParamBase::operator=(const WorkloadExtraParamBase& other)
    {
        return *this;
    }
}