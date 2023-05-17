#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::FACEBOOK_WORKLOAD_NAME("facebook");

    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGenerator(const std::string& workload_name, const uint32_t& global_client_idx)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == FACEBOOK_WORKLOAD_NAME)
        {
            workload_ptr = new FacebookWorkloadWrapper();
        }
        else
        {
            std::ostringstream oss;
            oss << "workload " << workload_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(workload_ptr != NULL);
        workload_ptr->validate(global_client_idx); // validate workload before generating each request

        return workload_ptr;
    }

    WorkloadWrapperBase::WorkloadWrapperBase()
    {
        is_valid_ = false;
    }

    WorkloadWrapperBase::~WorkloadWrapperBase() {}

    void WorkloadWrapperBase::validate(const uint32_t& global_client_idx)
    {
        if (!is_valid_)
        {
            initWorkloadParameters_();
            overwriteWorkloadParameters_();
            createWorkloadGenerator_(global_client_idx);

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(kClassName, "duplicate invoke of validate()!");
        }
        return;
    }

    WorkloadItem WorkloadWrapperBase::generateItem(std::mt19937_64& request_randgen)
    {
        checkIsValid();
        return generateItemInternal_(request_randgen);
    }

    void WorkloadWrapperBase::checkIsValid()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "not invoke validate() yet!");
            exit(-1);
        }
        return;
    }
}