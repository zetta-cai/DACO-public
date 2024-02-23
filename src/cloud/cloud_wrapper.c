#include "cloud/cloud_wrapper.h"

#include <assert.h>
#include <sstream>

#include "cloud/data_server/data_server.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CloudWrapper::kClassName("CloudWrapper");

    CloudWrapperParam::CloudWrapperParam()
    {
        cloud_idx_ = 0;
        cloud_cli_ptr_ = NULL;
    }

    CloudWrapperParam::CloudWrapperParam(const uint32_t& cloud_idx, CloudCLI* cloud_cli_ptr)
    {
        assert(cloud_cli_ptr != NULL);

        cloud_idx_ = cloud_idx;
        cloud_cli_ptr_ = cloud_cli_ptr;
    }

    CloudWrapperParam::~CloudWrapperParam()
    {
        // NOTE: NO need to release cloud_cli_ptr_, which is maintained outside CloudWrapperParam
    }

    uint32_t CloudWrapperParam::getCloudIdx() const
    {
        return cloud_idx_;
    }
    
    CloudCLI* CloudWrapperParam::getCloudCLIPtr() const
    {
        assert(cloud_cli_ptr_ != NULL);
        return cloud_cli_ptr_;
    }

    CloudWrapperParam& CloudWrapperParam::operator=(const CloudWrapperParam& other)
    {
        cloud_idx_ = other.cloud_idx_;
        cloud_cli_ptr_ = other.cloud_cli_ptr_;
        return *this;
    }

    void* CloudWrapper::launchCloud(void* cloud_wrapper_param_ptr)
    {
        assert(cloud_wrapper_param_ptr != NULL);
        CloudWrapperParam& cloud_wrapper_param = *((CloudWrapperParam*)cloud_wrapper_param_ptr);

        uint32_t cloud_idx = cloud_wrapper_param.getCloudIdx();
        CloudCLI* cloud_cli_ptr = cloud_wrapper_param.getCloudCLIPtr();

        CloudWrapper local_cloud(cloud_idx, cloud_cli_ptr->getCloudStorage(), cloud_cli_ptr->getKeycnt(), cloud_cli_ptr->getPropagationLatencyEdgecloudUs(), cloud_cli_ptr->getWorkloadName());
        local_cloud.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    CloudWrapper::CloudWrapper(const uint32_t& cloud_idx, const std::string& cloud_storage, const uint32_t& keycnt, const uint32_t& propagation_latency_edgecloud_us, const std::string& workload_name) : NodeWrapperBase(NodeWrapperBase::CLOUD_NODE_ROLE, cloud_idx, 1, true)
    {
        assert(cloud_idx == 0); // TODO: only support 1 cloud node now!

        // Different different clouds if any
        std::ostringstream oss;
        oss << kClassName << " cloud" << cloud_idx;
        instance_name_ = oss.str();

        // Create workload generator for warmup speedup (skip disk I/O latency in cloud)
        // NOTE: ONLY need keycnt and workload name to generate dataset items or locate corresponding dataset file, yet NOT need other parameters, as we only use dataset key-value pairs instead of workload items
        const uint32_t tmp_clientcnt = 0; // No need workload items
        const uint32_t tmp_client_idx = 0; // No need workload items
        const uint32_t tmp_perclient_workercnt = 0; // No need workload items
        const uint32_t tmp_perclient_opcnt = 0; // No need workload items
        workload_generator_ptr_ = WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(tmp_clientcnt, tmp_client_idx, keycnt, tmp_perclient_opcnt, tmp_perclient_workercnt, workload_name, covered::WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD); // Track dataset items to support quick operations for warmup speedup (skip disk I/O latency in cloud)
        assert(workload_generator_ptr_ != NULL);
        
        // Open local RocksDB KVS (maybe time-consuming -> introduce NodeParamBase::node_initialized_)
        cloud_rocksdb_ptr_ = new RocksdbWrapper(cloud_idx, cloud_storage, Util::getCloudRocksdbDirpath(keycnt, workload_name, cloud_idx));
        assert(cloud_rocksdb_ptr_ != NULL);

        // Allocate cloud-to-edge propagation simulator param
        cloud_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_edgecloud_us, Config::getPropagationItemBufferSizeCloudToedge());
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);

        // Sub-threads
        cloud_toedge_propagation_simulator_thread_ = 0;
        data_server_thread_ = 0;
    }
        
    CloudWrapper::~CloudWrapper()
    {
        // Release workload generator
        assert(workload_generator_ptr_ != NULL);
        delete workload_generator_ptr_;
        workload_generator_ptr_ = NULL;
        
        // Close local RocksDB KVS
        assert(cloud_rocksdb_ptr_ != NULL);
        delete cloud_rocksdb_ptr_;
        cloud_rocksdb_ptr_ = NULL;

        // Release cloud-to-edge propataion simulator param
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);
        delete cloud_toedge_propagation_simulator_param_ptr_;
        cloud_toedge_propagation_simulator_param_ptr_ = NULL;
    }

    WorkloadWrapperBase* CloudWrapper::getWorkloadGeneratorPtr() const
    {
        assert(workload_generator_ptr_ != NULL);
        return workload_generator_ptr_;
    }

    RocksdbWrapper* CloudWrapper::getCloudRocksdbPtr() const
    {
        assert(cloud_rocksdb_ptr_ != NULL);
        return cloud_rocksdb_ptr_;
    }
    
    PropagationSimulatorParam* CloudWrapper::getCloudToedgePropagationSimulatorParamPtr() const
    {
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);
        return cloud_toedge_propagation_simulator_param_ptr_;
    }

    void CloudWrapper::initialize_()
    {
        checkPointers_();

        // Launch cloud-to-client propagation simulator
        //int pthread_returncode = pthread_create(&cloud_toedge_propagation_simulator_thread, NULL, PropagationSimulator::launchPropagationSimulator, (void*)cloud_toedge_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch cloud-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "cloud-toedge-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::CLOUD_THREAD_ROLE, tmp_thread_name, &cloud_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)cloud_toedge_propagation_simulator_param_ptr_);

        // Launch a data server thread in the local cloud
        //pthread_returncode = pthread_create(&data_server_thread_, NULL, DataServer::launchDataServer, (void*)(&cloud_worker_param));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch cloud data server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "cloud-data-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::CLOUD_THREAD_ROLE, tmp_thread_name, &data_server_thread_, DataServer::launchDataServer, (void*)(this));

        return;
    }

    void CloudWrapper::processFinishrunRequest_()
    {
        checkPointers_();

        // Mark the current node as NOT running to finish benchmark
        resetNodeRunning_();

        // Send back SimpleFinishrunResponse to evaluator
        SimpleFinishrunResponse simple_finishrun_response(node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&simple_finishrun_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    void CloudWrapper::processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        assert(control_request_ptr != NULL);
        
        std::ostringstream oss;
        oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for startInternal_()";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void CloudWrapper::cleanup_()
    {
        checkPointers_();

        // Wait cloud-to-edge propagation simulator
        int pthread_returncode = pthread_join(cloud_toedge_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join client-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait cloud data server
        pthread_returncode = pthread_join(data_server_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join cloud data server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
    }

    void CloudWrapper::checkPointers_() const
    {
        NodeWrapperBase::checkPointers_();

        assert(workload_generator_ptr_ != NULL);
        assert(cloud_rocksdb_ptr_ != NULL);
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);

        return;
    }
}