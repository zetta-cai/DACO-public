#include "workload/workload_base.h"

const std::string covered::WorkloadBase::kClassName = "WorkloadBase";

covered::WorkloadBase::WorkloadBase()
{
    is_valid_ = false;
}

covered::WorkloadBase::~WorkloadBase() {}

void covered::WorkloadBase::validate()
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
}

covered::Request covered::WorkloadBase::generateReq()
{
    checkIsValid();
    return generateReqInternal();
}

void covered::WorkloadBase::checkIsValid()
{
    if (!is_valid_)
    {
        covered::Util::dumpErrorMsg(kClassName, "not invoke validate()!");
        exit(-1);
    }
    return;
}