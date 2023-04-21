#include "common/util.h"
#include "worker_param.h"

namespace covered
{
    const std::string WorkerParam::kClassName("WorkerParam");

    WorkerParam::WorkerParam()
    {
        local_client_param_ptr_ = NULL;
        local_worker_idx_ = 0;
    }

    WorkerParam::WorkerParam(ClientParam* local_client_param_ptr, uint32_t local_worker_idx)
    {
        if (local_client_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_client_param_ptr is NULL!");
            exit(1);
        }
        local_client_param_ptr_ = local_client_param_ptr;
        local_worker_idx_ = local_worker_idx;
    }

    WorkerParam::~WorkerParam()
    {
        // NOTE: no need to delete local_client_param_ptr_, as it is maintained outside WorkerParam
    }

    const WorkerParam& WorkerParam::operator=(const WorkerParam& other)
    {
        if (other.local_client_param_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.local_client_param_ptr_ is NULL!");
            exit(1);
        }
        local_client_param_ptr_ = other.local_client_param_ptr_;
        local_worker_idx_ = other.local_worker_idx_;
        return *this;
    }

    ClientParam* WorkerParam::getLocalClientParamPtr()
    {
        return local_client_param_ptr_;
    }

    uint32_t WorkerParam::getLocalWorkerIdx()
    {
        return local_worker_idx_;
    }
}