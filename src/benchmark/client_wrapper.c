#include "benchmark/client_wrapper.h"

#include <assert.h> // assert
#include <pthread.h>
#include <sstream>

#include "benchmark/client_worker_param.h"
#include "benchmark/client_worker_wrapper.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string ClientWrapper::kClassName("ClientWrapper");

    void* ClientWrapper::launchClient(void* client_param_ptr)
    {
        ClientWrapper local_client(Param::getClientcnt(), Param::getEdgecnt(), Param::getKeycnt(), Param::getOpcnt(), Param::getPerclientWorkercnt(), Param::getPropagationLatencyClientedge(), Param::getWorkloadName(), (ClientParam*)client_param_ptr);
        local_client.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    ClientWrapper::ClientWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const uint32_t& propagation_latency_clientedge, const std::string& workload_name, ClientParam* client_param_ptr) : clientcnt_(clientcnt), edgecnt_(edgecnt), perclient_workercnt_(perclient_workercnt), client_param_ptr_(client_param_ptr)
    {
        if (client_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_param_ptr is NULL!");
            exit(1);
        }
        uint32_t client_idx = client_param_ptr->getNodeIdx();

        // Differentiate different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // Create workload generator for the client
        // NOTE: creating workload generator needs time, so we introduce NodeParamBase::node_initialized_
        workload_generator_ptr_ = WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, opcnt, perclient_workercnt, workload_name);
        assert(workload_generator_ptr_ != NULL);

        // Create statistics tracker for the client
        client_statistics_tracker_ptr_ = new covered::ClientStatisticsTracker(perclient_workercnt, client_idx);
        assert(client_statistics_tracker_ptr_ != NULL);

        // Allocate client-to-edge propagation simulator param
        client_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam(propagation_latency_clientedge, (NodeParamBase*)client_param_ptr, Config::getPropagationItemBufferSizeClientToedge());
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);
    }

    ClientWrapper::~ClientWrapper()
    {
        // NOTE: no need to delete client_param_ptr_, as it is maintained outside ClientWrapper

        // Release workload generator
        assert(workload_generator_ptr_ != NULL);
        delete workload_generator_ptr_;
        workload_generator_ptr_ = NULL;

        // Release client statistics
        assert(client_statistics_tracker_ptr_ != NULL);
        delete client_statistics_tracker_ptr_;
        client_statistics_tracker_ptr_ = NULL;

        // Release client-to-edge propagation simulator param
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);
        delete client_toedge_propagation_simulator_param_ptr_;
        client_toedge_propagation_simulator_param_ptr_ = NULL;
    }

    void ClientWrapper::start()
    {
        checkPointers_();
        
        int pthread_returncode = 0;

        // Launch client-to-edge propagation simulator
        pthread_t client_toedge_propagation_simulator_thread;
        //pthread_returncode = pthread_create(&client_toedge_propagation_simulator_thread, NULL, PropagationSimulator::launchPropagationSimulator, (void*)client_toedge_propagation_simulator_param_ptr_);
        pthread_returncode = Util::pthreadCreateHighPriority(&client_toedge_propagation_simulator_thread, PropagationSimulator::launchPropagationSimulator, (void*)client_toedge_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch client-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Prepare perclient_workercnt worker parameters
        ClientWorkerParam client_worker_params[perclient_workercnt_];
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            ClientWorkerParam local_client_worker_param(this, local_client_worker_idx);
            client_worker_params[local_client_worker_idx] = local_client_worker_param;
        }

        // Launch perclient_workercnt worker threads in the local client
        pthread_t client_worker_threads[perclient_workercnt_];
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            //pthread_returncode = pthread_create(&client_worker_threads[local_client_worker_idx], NULL, ClientWorkerWrapper::launchClientWorker, (void*)(&(client_worker_params[local_client_worker_idx])));
            pthread_returncode = Util::pthreadCreateHighPriority(&client_worker_threads[local_client_worker_idx], ClientWorkerWrapper::launchClientWorker, (void*)(&(client_worker_params[local_client_worker_idx])));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << " failed to launch worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // After all time-consuming initialization
        client_param_ptr_->setNodeInitialized();

        // Wait client-to-edge propagation simulator
        pthread_returncode = pthread_join(client_toedge_propagation_simulator_thread, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join client-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for all local workers
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            pthread_returncode = pthread_join(client_worker_threads[local_client_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << " failed to join client worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Get per-client statistics file path
        assert(client_param_ptr_ != NULL);
        uint32_t client_idx = client_param_ptr_->getNodeIdx();
        std::string client_statistics_filepath = Util::getClientStatisticsFilepath(client_idx);

        // Dump per-client statistics
        assert(client_statistics_tracker_ptr_ != NULL);
        client_statistics_tracker_ptr_->dump(client_statistics_filepath);

        return;
    }

    void ClientWrapper::checkPointers_() const
    {
        assert(client_param_ptr_ != NULL);
        assert(workload_generator_ptr_ != NULL);
        assert(client_statistics_tracker_ptr_ != NULL);
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);

        return;
    }
}