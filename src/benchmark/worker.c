#include "benchmark/worker.h"

#include <sstream>
#include <assert.h> // assert

#include "common/util.h"
#include "common/request.h"
#include "benchmark/client_param.h"
#include "benchmark/benchmark_util.h"

namespace covered
{
    const std::string Worker::kClassName("Worker");

    void* Worker::launchWorker(void* local_worker_param_ptr)
    {
        Worker local_worker((WorkerParam*)local_worker_param_ptr);
        local_worker.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    Worker::Worker(WorkerParam* local_worker_param_ptr)
    {
        if (local_worker_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_worker_param_ptr is NULL!");
            exit(1);
        }
        local_worker_param_ptr_ = local_worker_param_ptr;

        // Each per-client worker uses global_worker_idx to create a random generator and get different requests
        ClientParam* local_client_param_ptr = local_worker_param_ptr_->getLocalClientParamPtr();
        assert(local_client_param_ptr != NULL);
        const uint32_t global_client_idx = local_client_param_ptr->getGlobalClientIdx();
        const uint32_t local_worker_idx = local_worker_param_ptr_->getLocalWorkerIdx();
        uint32_t global_worker_idx = Util::getGlobalWorkerIdx(global_client_idx, local_worker_idx);
        request_randgen_ptr_ = new std::mt19937_64(global_worker_idx);
        if (request_randgen_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "failed to create a random generator for requests!");
            exit(1);
        }
    }
    
    Worker::~Worker()
    {
        // NOTE: no need to delete local_worker_param_ptr_, as it is maintained outside Worker
        if (request_randgen_ptr_ != NULL)
        {
            delete request_randgen_ptr_;
        }
    }

    void Worker::start()
    {
        assert(local_worker_param_ptr_ != NULL);
        ClientParam* local_client_param_ptr = local_worker_param_ptr_->getLocalClientParamPtr();
        assert(local_client_param_ptr != NULL);
        WorkloadBase* workload_generator_ptr = local_client_param_ptr->getWorkloadGeneratorPtr();
        assert(workload_generator_ptr != NULL);

        // Block until local_client_running_ becomes true
        while (!local_client_param_ptr->isClientRunning()) {}

        // Current worker thread start to issue requests and receive responses
        while (local_client_param_ptr->isClientRunning())
        {
            // Generate key-value request based on a specific workload
            Request current_request = workload_generator_ptr->generateReq(*request_randgen_ptr_);

            std::ostringstream oss;
            oss << "keystr: " << current_request.getKey().getKeystr() << "; valuesize: " << current_request.getValue().getValuesize() << std::endl;
            Util::dumpNormalMsg(kClassName, oss.str());
            break;

            // TODO: convert the request into UDP packets
            // TODO: communicate with local edge node listening on a specific port for the worker
        }

        return;
    }
}