/*
 * ClientWorkerParam: parameters to launch a worker in a local client.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_WORKER_PARAM_H
#define CLIENT_WORKER_PARAM_H

#include <string>

namespace covered
{
    class ClientWorkerParam;
}

#include "benchmark/client_wrapper.h"

namespace covered
{
    class ClientWorkerParam
    {
    public:
        ClientWorkerParam();
        ClientWorkerParam(ClientWrapper* client_wrapper_ptr, uint32_t local_client_worker_idx);
        ~ClientWorkerParam();

        const ClientWorkerParam& operator=(const ClientWorkerParam& other);

        ClientWrapper* getClientWrapperPtr();
        uint32_t getLocalClientWorkerIdx();
    private:
        static const std::string kClassName;

        ClientWrapper* client_wrapper_ptr_;
        uint32_t local_client_worker_idx_;
    };
}

#endif