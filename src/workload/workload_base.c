#include "common/util.h"
#include "workload/workload_base.h"

namespace covered
{
    const std::string WorkloadBase::kClassName = "WorkloadBase";

    WorkloadBase::WorkloadBase()
    {
        is_valid_ = false;
    }

    WorkloadBase::~WorkloadBase() {}

    void WorkloadBase::validate()
    {
        if (!is_valid_)
        {
            initWorkloadParameters();
            overwriteWorkloadParameters();
            createWorkloadGenerator();

            is_valid_ = true;
        }
        else
        {
            dumpWarnMsg(kClassName, "duplicate invoke of validate()!");
        }
        return;
    }

    Request WorkloadBase::generateReq()
    {
        checkIsValid();
        return generateReqInternal();
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