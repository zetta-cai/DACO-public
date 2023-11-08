#include "benchmark/client_wrapper.h"

#include <assert.h> // assert
#include <pthread.h>
#include <sstream>

#include "benchmark/client_worker_wrapper.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    const std::string ClientWrapper::kClassName("ClientWrapper");

    ClientWrapperParam::ClientWrapperParam()
    {
        client_idx_ = 0;
        client_cli_ptr_ = NULL;
    }

    ClientWrapperParam::ClientWrapperParam(const uint32_t& client_idx, ClientCLI* client_cli_ptr)
    {
        assert(client_cli_ptr != NULL);

        client_idx_ = client_idx;
        client_cli_ptr_ = client_cli_ptr;
    }

    ClientWrapperParam::~ClientWrapperParam()
    {
        // NOTE: NO need to release client_cli_ptr_, which is maintained outside ClientWrapperParam
    }

    uint32_t ClientWrapperParam::getClientIdx() const
    {
        return client_idx_;
    }
    
    ClientCLI* ClientWrapperParam::getClientCLIPtr() const
    {
        assert(client_cli_ptr_ != NULL);
        return client_cli_ptr_;
    }

    ClientWrapperParam& ClientWrapperParam::operator=(const ClientWrapperParam& other)
    {
        client_idx_ = other.client_idx_;
        client_cli_ptr_ = other.client_cli_ptr_;
        return *this;
    }

    void* ClientWrapper::launchClient(void* client_wrapper_param_ptr)
    {
        assert(client_wrapper_param_ptr != NULL);
        ClientWrapperParam& client_wrapper_param = *((ClientWrapperParam*)client_wrapper_param_ptr);

        uint32_t client_idx = client_wrapper_param.getClientIdx();
        ClientCLI* client_cli_ptr = client_wrapper_param.getClientCLIPtr();
        
        ClientWrapper local_client(client_cli_ptr->getCapacityBytes(), client_idx, client_cli_ptr->getClientcnt(), client_cli_ptr->isWarmupSpeedup(), client_cli_ptr->getEdgecnt(), client_cli_ptr->getKeycnt(), client_cli_ptr->getPerclientOpcnt(), client_cli_ptr->getPerclientWorkercnt(), client_cli_ptr->getPropagationLatencyClientedgeUs(), client_cli_ptr->getWorkloadName());
        local_client.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    ClientWrapper::ClientWrapper(const uint64_t& capacity_bytes, const uint32_t& client_idx, const uint32_t& clientcnt, const bool& is_warmup_speedup, const uint32_t& edgecnt, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const uint32_t& propagation_latency_clientedge_us, const std::string& workload_name) : NodeWrapperBase(NodeWrapperBase::CLIENT_NODE_ROLE, client_idx, clientcnt, false), is_warmup_speedup_(is_warmup_speedup), capacity_bytes_(capacity_bytes), edgecnt_(edgecnt), perclient_workercnt_(perclient_workercnt), is_warmup_phase_(true)
    {
        // Differentiate different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // Create workload generator for the client
        // NOTE: creating workload generator needs time, so we introduce NodeParamBase::node_initialized_
        workload_generator_ptr_ = WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(capacity_bytes_, clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name);
        Util::dumpVariablesForDebug(instance_name_, 4, "average dataset key size:", std::to_string(workload_generator_ptr_->getAvgDatasetKeysize()).c_str(), "average dataset value size:", std::to_string(workload_generator_ptr_->getAvgDatasetValuesize()).c_str());
        Util::dumpVariablesForDebug(instance_name_, 4, "min dataset key size:", std::to_string(workload_generator_ptr_->getMinDatasetKeysize()).c_str(), "min dataset value size:", std::to_string(workload_generator_ptr_->getMinDatasetValuesize()).c_str());
        Util::dumpVariablesForDebug(instance_name_, 4, "max dataset key size:", std::to_string(workload_generator_ptr_->getMaxDatasetKeysize()).c_str(), "max dataset value size:", std::to_string(workload_generator_ptr_->getMaxDatasetValuesize()).c_str());
        assert(workload_generator_ptr_ != NULL);

        // Create statistics tracker for the client
        client_statistics_tracker_ptr_ = new covered::ClientStatisticsTracker(perclient_workercnt, client_idx);
        assert(client_statistics_tracker_ptr_ != NULL);

        // Allocate client-to-edge propagation simulator param
        client_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_clientedge_us, Config::getPropagationItemBufferSizeClientToedge());
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);

        // Prepare perclient_workercnt worker parameters
        client_worker_params_ = new ClientWorkerParam[perclient_workercnt];
        assert(client_worker_params_ != NULL);
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt; local_client_worker_idx++)
        {
            ClientWorkerParam local_client_worker_param(this, local_client_worker_idx);
            client_worker_params_[local_client_worker_idx] = local_client_worker_param;
        }

        // Sub-threads
        client_toedge_propagation_simulator_thread_ = 0;
        client_worker_threads_ = new pthread_t[perclient_workercnt];
        assert(client_worker_threads_ != NULL);
        memset(client_worker_threads_, 0, perclient_workercnt * sizeof(pthread_t));
    }

    ClientWrapper::~ClientWrapper()
    {
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

        // Release client worker parameters
        assert(client_worker_params_ != NULL);
        delete[] client_worker_params_;
        client_worker_params_ = NULL;

        // Release sub-threads
        assert(client_worker_threads_ != NULL);
        delete[] client_worker_threads_;
        client_worker_threads_ = NULL;
    }

    // (1) Const getters

    uint32_t ClientWrapper::getEdgeCnt() const
    {
        return edgecnt_;
    }

    bool ClientWrapper::isWarmupSpeedup() const
    {
        return is_warmup_speedup_;
    }
    
    uint32_t ClientWrapper::getPerclientWorkercnt() const
    {
        return perclient_workercnt_;
    }

    bool ClientWrapper::isWarmupPhase() const
    {
        return is_warmup_phase_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    WorkloadWrapperBase* ClientWrapper::getWorkloadWrapperPtr() const
    {
        assert(workload_generator_ptr_ != NULL);
        return workload_generator_ptr_;
    }

    ClientStatisticsTracker* ClientWrapper::getClientStatisticsTrackerPtr() const
    {
        assert(client_statistics_tracker_ptr_ != NULL);
        return client_statistics_tracker_ptr_;
    }

    PropagationSimulatorParam* ClientWrapper::getClientToedgePropagationSimulatorParamPtr() const
    {
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);
        return client_toedge_propagation_simulator_param_ptr_;
    }

    // (2) Process benchmark control messages

    void ClientWrapper::initialize_()
    {
        checkPointers_();
        
        // Launch client-to-edge propagation simulator
        //int pthread_returncode = pthread_create(&client_toedge_propagation_simulator_thread, NULL, PropagationSimulator::launchPropagationSimulator, (void*)client_toedge_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch client-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "client-toedge-propagation-simluator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &client_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)client_toedge_propagation_simulator_param_ptr_);

        // Launch perclient_workercnt worker threads in the local client
        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            //pthread_returncode = pthread_create(&client_worker_threads[local_client_worker_idx], NULL, launchClientWorker, (void*)(&(client_worker_params[local_client_worker_idx])));
            // if (pthread_returncode != 0)
            // {
            //     std::ostringstream oss;
            //     oss << " failed to launch worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            //     Util::dumpErrorMsg(instance_name_, oss.str());
            //     exit(1);
            // }
            tmp_thread_name = "client-worker-" + std::to_string(node_idx_) + "-" + std::to_string(local_client_worker_idx);
            ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &client_worker_threads_[local_client_worker_idx], ClientWorkerWrapper::launchClientWorker, (void*)(&(client_worker_params_[local_client_worker_idx])));
        }

        return;
    }

    void ClientWrapper::processFinishrunRequest_()
    {
        checkPointers_();

        // Mark the current node as NOT running to finish benchmark
        resetNodeRunning_();

        // Aggregate cur-slot/stable client raw statistics into last-slot/stable client aggregated statistics
        ClientAggregatedStatistics lastslot_client_aggregated_statistics;
        ClientAggregatedStatistics stable_client_aggregated_statistics;
        uint32_t last_slot_idx = client_statistics_tracker_ptr_->aggregateForFinishrun(lastslot_client_aggregated_statistics, stable_client_aggregated_statistics);

        // Send back FinishrunResponse to evaluator
        FinishrunResponse finishrun_response(last_slot_idx, lastslot_client_aggregated_statistics, stable_client_aggregated_statistics, node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&finishrun_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    void ClientWrapper::processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        assert(control_request_ptr != NULL);
        
        MessageType control_request_msg_type = control_request_ptr->getMessageType();
        if (control_request_msg_type == MessageType::kSwitchSlotRequest)
        {
            processSwitchSlotRequest_(control_request_ptr); // Increase cur_slot_idx_ in ClientStatisticsTracker and return cur-slot client aggregated statistics
        }
        else if (control_request_msg_type == MessageType::kFinishWarmupRequest)
        {
            processWarmupRequest_(); // Mark is_warmup_phase_ as false
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(control_request_msg_type) << " for startInternal_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
    }

    void ClientWrapper::cleanup_()
    {
        checkPointers_();

        int pthread_returncode = 0;

        // Wait client-to-edge propagation simulator
        pthread_returncode = pthread_join(client_toedge_propagation_simulator_thread_, NULL);
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
            pthread_returncode = pthread_join(client_worker_threads_[local_client_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << " failed to join client worker " << local_client_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        return;
    }

    void ClientWrapper::processSwitchSlotRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL);

        const SwitchSlotRequest* const switch_slot_request_ptr = static_cast<const SwitchSlotRequest*>(control_request_ptr);
        uint32_t target_slot_idx = switch_slot_request_ptr->getTargetSlotIdx();

        // Switch cur-slot client raw statistics and aggregate
        ClientAggregatedStatistics curslot_client_aggregated_statistics = client_statistics_tracker_ptr_->switchCurslotForClientRawStatistics(target_slot_idx);

        // Send back SwitchSlotResponse to evaluator
        SwitchSlotResponse switch_slot_response(target_slot_idx, curslot_client_aggregated_statistics, node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&switch_slot_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    void ClientWrapper::processWarmupRequest_()
    {
        // Finish warmup phase to start stresstest phase
        finishWarmupPhase_();

        // Send back FinishWarmupResponse to evaluator
        FinishWarmupResponse finish_warmup_response(node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&finish_warmup_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    // (3) Other utility functions

    void ClientWrapper::finishWarmupPhase_()
    {
        is_warmup_phase_.store(false, Util::STORE_CONCURRENCY_ORDER);
        return;
    }

    void ClientWrapper::checkPointers_() const
    {
        NodeWrapperBase::checkPointers_();

        assert(workload_generator_ptr_ != NULL);
        assert(client_statistics_tracker_ptr_ != NULL);
        assert(client_toedge_propagation_simulator_param_ptr_ != NULL);

        return;
    }
}