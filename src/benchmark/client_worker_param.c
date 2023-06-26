#include "benchmark/client_worker_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientWorkerParam::kClassName("ClientWorkerParam");

    ClientWorkerParam::ClientWorkerParam()
    {
        client_param_ptr_ = NULL;
        local_client_worker_idx_ = 0;
    }

    ClientWorkerParam::ClientWorkerParam(ClientParam* client_param_ptr, uint32_t local_client_worker_idx)
    {
        if (client_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_param_ptr is NULL!");
            exit(1);
        }
        client_param_ptr_ = client_param_ptr;
        local_client_worker_idx_ = local_client_worker_idx;
    }

    ClientWorkerParam::~ClientWorkerParam()
    {
        // NOTE: no need to delete client_param_ptr_, as it is maintained outside ClientWorkerParam
    }

    const ClientWorkerParam& ClientWorkerParam::operator=(const ClientWorkerParam& other)
    {
        if (other.client_param_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.client_param_ptr_ is NULL!");
            exit(1);
        }
        client_param_ptr_ = other.client_param_ptr_;
        local_client_worker_idx_ = other.local_client_worker_idx_;
        return *this;
    }

    ClientParam* ClientWorkerParam::getClientParamPtr()
    {
        return client_param_ptr_;
    }

    uint32_t ClientWorkerParam::getLocalClientWorkerIdx()
    {
        return local_client_worker_idx_;
    }
}