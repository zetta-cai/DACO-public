#include "benchmark/worker.h"

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
    }
    
    Worker::~Worker()
    {
        // NOTE: no need to delete local_worker_param_ptr_, as it is maintained outside Worker
    }

    void Worker::start()
    {
        // TODO: Block with a while loop until local_client_running_ = true

        // TODO: for each sub-thread, within a while loop until local_client_running_ = false

            // TODO: generate key-value request based on a specific workload

        // TODO: at the end of duration, notify main thread by pthread_exit
    }
}