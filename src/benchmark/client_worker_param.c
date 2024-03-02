#include "benchmark/client_worker_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientWorkerParam::kClassName("ClientWorkerParam");

    ClientWorkerParam::ClientWorkerParam() : SubthreadParamBase()
    {
        client_wrapper_ptr_ = NULL;
        local_client_worker_idx_ = 0;
    }

    ClientWorkerParam::ClientWorkerParam(ClientWrapper* client_wrapper_ptr, uint32_t local_client_worker_idx) : SubthreadParamBase()
    {
        if (client_wrapper_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_wrapper_ptr is NULL!");
            exit(1);
        }
        client_wrapper_ptr_ = client_wrapper_ptr;
        local_client_worker_idx_ = local_client_worker_idx;
    }

    ClientWorkerParam::~ClientWorkerParam()
    {
        // NOTE: no need to delete client_wrapper_ptr_, as it is maintained outside ClientWorkerParam
    }

    const ClientWorkerParam& ClientWorkerParam::operator=(const ClientWorkerParam& other)
    {
        SubthreadParamBase::operator=(other);

        if (other.client_wrapper_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.client_wrapper_ptr_ is NULL!");
            exit(1);
        }
        client_wrapper_ptr_ = other.client_wrapper_ptr_;
        local_client_worker_idx_ = other.local_client_worker_idx_;
        return *this;
    }

    ClientWrapper* ClientWorkerParam::getClientWrapperPtr()
    {
        assert(client_wrapper_ptr_ != NULL);
        return client_wrapper_ptr_;
    }

    uint32_t ClientWorkerParam::getLocalClientWorkerIdx()
    {
        return local_client_worker_idx_;
    }
}