#include "cloud/cloud_wrapper.h"

#include <assert.h>
#include <sstream>

#include "cloud/data_server/data_server.h"
#include "common/config.h"
#include "common/param/cloud_param.h"
#include "common/param/propagation_param.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CloudWrapper::kClassName("CloudWrapper");

    void* CloudWrapper::launchCloud(void* cloud_idx_ptr)
    {
        assert(cloud_idx_ptr != NULL);
        uint32_t cloud_idx = *((uint32_t*)cloud_idx_ptr);

        CloudWrapper local_cloud(cloud_idx, CloudParam::getCloudStorage(), PropagationParam::getPropagationLatencyEdgecloudUs());
        local_cloud.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    CloudWrapper::CloudWrapper(const uint32_t& cloud_idx, const std::string& cloud_storage, const uint32_t& propagation_latency_edgecloud_us) : NodeWrapperBase(NodeWrapperBase::CLOUD_NODE_ROLE, cloud_idx, 1, true)
    {
        assert(cloud_idx == 0); // TODO: only support 1 cloud node now!

        // Different different clouds if any
        std::ostringstream oss;
        oss << kClassName << " cloud" << cloud_idx;
        instance_name_ = oss.str();
        
        // Open local RocksDB KVS (maybe time-consuming -> introduce NodeParamBase::node_initialized_)
        cloud_rocksdb_ptr_ = new RocksdbWrapper(cloud_idx, cloud_storage, Util::getCloudRocksdbDirpath(cloud_idx));
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
        // Close local RocksDB KVS
        assert(cloud_rocksdb_ptr_ != NULL);
        delete cloud_rocksdb_ptr_;
        cloud_rocksdb_ptr_ = NULL;

        // Release cloud-to-edge propataion simulator param
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);
        delete cloud_toedge_propagation_simulator_param_ptr_;
        cloud_toedge_propagation_simulator_param_ptr_ = NULL;
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

        int pthread_returncode = 0;

        // Launch cloud-to-client propagation simulator
        //pthread_returncode = pthread_create(&cloud_toedge_propagation_simulator_thread, NULL, PropagationSimulator::launchPropagationSimulator, (void*)cloud_toedge_propagation_simulator_param_ptr_);
        pthread_returncode = Util::pthreadCreateHighPriority(&cloud_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)cloud_toedge_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch cloud-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch a data server thread in the local cloud
        //pthread_returncode = pthread_create(&data_server_thread_, NULL, DataServer::launchDataServer, (void*)(&cloud_worker_param));
        pthread_returncode = Util::pthreadCreateHighPriority(&data_server_thread_, DataServer::launchDataServer, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch cloud data server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

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

        assert(cloud_rocksdb_ptr_ != NULL);
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);

        return;
    }
}