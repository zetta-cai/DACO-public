#include "benchmark/client_wrapper.h"

#include <assert.h> // assert
#include <pthread.h>
#include <sstream>

#include "benchmark/client_worker_param.h"
#include "benchmark/client_worker_wrapper.h"
#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientWrapper::kClassName("ClientWrapper");

    void* ClientWrapper::launchClientWorker_(void* client_worker_param_ptr)
    {
        ClientWorkerWrapper local_client_worker((ClientWorkerParam*)client_worker_param_ptr);
        local_client_worker.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    ClientWrapper::ClientWrapper(const uint32_t& perclient_workercnt, ClientParam* client_param_ptr) : perclient_workercnt_(perclient_workercnt)
    {
        if (client_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_param_ptr is NULL!");
            exit(1);
        }

        // Differentiate different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_param_ptr->getClientIdx();
        instance_name_ = oss.str();
        
        client_param_ptr_ = client_param_ptr;
        assert(client_param_ptr_ != NULL);
    }

    ClientWrapper::~ClientWrapper()
    {
        // NOTE: no need to delete client_param_ptr_, as it is maintained outside ClientWrapper
    }

    void ClientWrapper::start()
    {
        pthread_t client_worker_threads[perclient_workercnt_];
        ClientWorkerParam client_worker_params[perclient_workercnt_];
        int pthread_returncode;
        assert(client_param_ptr_ != NULL);

        // Prepare perclient_workercnt worker parameters
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            ClientWorkerParam local_client_worker_param(client_param_ptr_, local_client_worker_idx);
            client_worker_params[local_client_worker_idx] = local_client_worker_param;
        }

        // Launch perclient_workercnt worker threads in the local client
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            pthread_returncode = pthread_create(&client_worker_threads[local_client_worker_idx], NULL, launchClientWorker_, (void*)(&(client_worker_params[local_client_worker_idx])));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "client " << client_param_ptr_->getClientIdx() << " failed to launch worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Wait for all local workers
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            pthread_returncode = pthread_join(client_worker_threads[local_client_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "client " << client_param_ptr_->getClientIdx() << " failed to join client worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Get per-client statistics file path
        assert(client_param_ptr_ != NULL);
        uint32_t client_idx = client_param_ptr_->getClientIdx();
        std::string client_statistics_filepath = Util::getClientStatisticsFilepath(client_idx);

        // Dump per-client statistics
        ClientStatisticsTracker* client_statistics_tracker_ptr_ = client_param_ptr_->getClientStatisticsTrackerPtr();
        assert(client_statistics_tracker_ptr_ != NULL);
        client_statistics_tracker_ptr_->dump(client_statistics_filepath);

        return;
    }
}