/*
 * WorkerParam: parameters to launch a worker in a local client.
 * 
 * By Siyuan Sheng (2023.04.20).
 */

#ifndef WORKER_PARAM_H
#define WORKER_PARAM_H

#include <string>
#include <atomic>

#include "benchmark/client_param.h"

namespace covered
{
    class WorkerParam
    {
    public:
        WorkerParam();
        WorkerParam(ClientParam* local_client_param_ptr, uint32_t local_worker_idx);
        ~WorkerParam();

        const WorkerParam& operator=(const WorkerParam& other);

        ClientParam* getLocalClientParamPtr();
        uint32_t getLocalWorkerIdx();
    private:
        static const std::string kClassName;

        ClientParam* local_client_param_ptr_;
        uint32_t local_worker_idx_;
    };
}

#endif