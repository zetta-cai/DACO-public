#include "workload/workload_base.h"

#include <sstream>

#include "common/util.h"
#include "workload/facebook_workload.h"

namespace covered
{
    const std::string WorkloadBase::FACEBOOK_WORKLOAD_NAME = "facebook";

    const std::string WorkloadBase::kClassName = "WorkloadBase";

    WorkloadBase* WorkloadBase::getWorkloadGenerator(std::string workload_name, const uint32_t& global_client_idx)
    {
        WorkloadBase* workload_ptr = NULL;
        if (workload_name == FACEBOOK_WORKLOAD_NAME)
        {
            workload_ptr = new FacebookWorkload();
        }
        else
        {
            std::ostringstream oss;
            oss << "workload " << workload_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        if (workload_ptr != NULL)
        {
            workload_ptr->validate(global_client_idx); // validate workload before generating each request
        }

        return workload_ptr;
    }

    WorkloadBase::WorkloadBase()
    {
        is_valid_ = false;
    }

    WorkloadBase::~WorkloadBase() {}

    void WorkloadBase::validate(const uint32_t& global_client_idx)
    {
        if (!is_valid_)
        {
            initWorkloadParameters();
            overwriteWorkloadParameters();
            createWorkloadGenerator(global_client_idx);

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(kClassName, "duplicate invoke of validate()!");
        }
        return;
    }

    Request WorkloadBase::generateReq(std::mt19937_64& request_randgen)
    {
        checkIsValid();
        return generateReqInternal(request_randgen);
    }

    void WorkloadBase::checkIsValid()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "not invoke validate() yet!");
            exit(-1);
        }
        return;
    }
}