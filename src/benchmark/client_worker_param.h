/*
 * ClientWorkerParam: parameters to launch a worker in a local client.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef CLIENT_WORKER_PARAM_H
#define CLIENT_WORKER_PARAM_H

#include <string>

#include "benchmark/client_param.h"

namespace covered
{
    class ClientWorkerParam
    {
    public:
        ClientWorkerParam();
        ClientWorkerParam(ClientParam* client_param_ptr, uint32_t local_client_worker_idx);
        ~ClientWorkerParam();

        const ClientWorkerParam& operator=(const ClientWorkerParam& other);

        ClientParam* getClientParamPtr();
        uint32_t getLocalClientWorkerIdx();
    private:
        static const std::string kClassName;

        ClientParam* client_param_ptr_;
        uint32_t local_client_worker_idx_;
    };
}

#endif