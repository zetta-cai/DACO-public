#include "benchmark/client_wrapper.h"

#include <pthread.h>
#include <sstream>
#include <assert.h> // assert

#include "common/util.h"
#include "common/param.h"
#include "benchmark/worker_param.h"
#include "benchmark/worker.h"

namespace covered
{
    const std::string ClientWrapper::kClassName("ClientWrapper");

    void* ClientWrapper::launchClient(void* local_client_param_ptr)
    {
        ClientWrapper local_client((ClientParam*)local_client_param_ptr);
        local_client.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    ClientWrapper::ClientWrapper(ClientParam* local_client_param_ptr)
    {
        if (local_client_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_client_param_ptr is NULL!");
            exit(1);
        }
        local_client_param_ptr_ = local_client_param_ptr;
    }

    ClientWrapper::~ClientWrapper()
    {
        // NOTE: no need to delete local_client_param_ptr_, as it is maintained outside ClientWrapper
    }

    void ClientWrapper::start()
    {
        uint32_t perclient_workercnt = Param::getPerclientWorkercnt();
        pthread_t local_worker_threads[perclient_workercnt];
        WorkerParam local_worker_params[perclient_workercnt];
        int pthread_returncode;
        assert(local_client_param_ptr_ != NULL);

        // Prepare perclient_workercnt worker parameters
        for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
        {
            WorkerParam local_worker_param(local_client_param_ptr_, local_worker_idx);
            local_worker_params[local_worker_idx] = local_worker_param;
        }

        // Launch perclient_workercnt worker threads in the local client
        for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
        {
            pthread_returncode = pthread_create(&local_worker_threads[local_worker_idx], NULL, covered::Worker::launchWorker, (void*)(&(local_worker_params[local_worker_idx])));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "client " << local_client_param_ptr_->getGlobalClientIdx() << " failed to launch worker " << local_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        // Wait for all local workers
        for (uint32_t local_worker_idx = 0; local_worker_idx < perclient_workercnt; local_worker_idx++)
        {
            pthread_returncode = pthread_join(local_worker_threads[local_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "client " << local_client_param_ptr_->getGlobalClientIdx() << " failed to join worker " << local_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        return;
    }
}