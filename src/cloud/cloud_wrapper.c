#include "cloud/cloud_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "message/data_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CloudWrapper::kClassName("CloudWrapper");

    void* CloudWrapper::launchCloud(void* cloud_param_ptr)
    {
        CloudWrapper local_cloud(Param::getCloudStorage(), Param::getPropagationLatencyEdgecloud(), (CloudParam*)cloud_param_ptr);
        local_cloud.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    CloudWrapper::CloudWrapper(const std::string& cloud_storage, const uint32_t& propagation_latency_edgecloud, CloudParam* cloud_param_ptr) : cloud_param_ptr_(cloud_param_ptr)
    {
        if (cloud_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "cloud_param_ptr is NULL!");
            exit(1);
        }
        const uint32_t cloud_idx = cloud_param_ptr->getNodeIdx();
        assert(cloud_idx == 0); // TODO: only support 1 cloud node now!

        // Different different clouds if any
        std::ostringstream oss;
        oss << kClassName << " cloud" << cloud_idx;
        instance_name_ = oss.str();
        
        // Open local RocksDB KVS (maybe time-consuming -> introduce NodeParamBase::node_initialized_)
        cloud_rocksdb_ptr_ = new RocksdbWrapper(cloud_storage, Util::getCloudRocksdbDirpath(cloud_idx), cloud_param_ptr);
        assert(cloud_rocksdb_ptr_ != NULL);

        // For receiving global requests

        // Get source address of cloud recvreq
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(cloud_idx);
        cloud_recvreq_source_addr_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // Prepare a socket server on recvreq port
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, cloud_recvreq_port);
        cloud_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);

        // Allocate cloud-to-edge propagation simulator param
        cloud_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam(propagation_latency_edgecloud, (NodeParamBase*)cloud_param_ptr, Config::getPropagationItemBufferSizeCloudToedge());
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);
    }
        
    CloudWrapper::~CloudWrapper()
    {
        // NOTE: no need to delete cloud_param_ptr, as it is maintained outside CloudWrapper

        // Close local RocksDB KVS
        assert(cloud_rocksdb_ptr_ != NULL);
        delete cloud_rocksdb_ptr_;
        cloud_rocksdb_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
        delete cloud_recvreq_socket_server_ptr_;
        cloud_recvreq_socket_server_ptr_ = NULL;

        // Release cloud-to-edge propataion simulator param
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);
        delete cloud_toedge_propagation_simulator_param_ptr_;
        cloud_toedge_propagation_simulator_param_ptr_ = NULL;
    }

    void CloudWrapper::start()
    {
        checkPointers_();

        int pthread_returncode = 0;
        bool is_finish = false; // Mark if local cloud node is finished

        // Launch cloud-to-client propagation simulator
        pthread_t cloud_toedge_propagation_simulator_thread;
        pthread_returncode = pthread_create(&cloud_toedge_propagation_simulator_thread, NULL, PropagationSimulator::launchPropagationSimulator, (void*)cloud_toedge_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch cloud-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // After all time-consuming initialization
        cloud_param_ptr_->setNodeInitialized();

        while (cloud_param_ptr_->isNodeRunning()) // cloud_running_ is set as true by default
        {
            // Receive the global request message
            DynamicArray global_request_msg_payload;
            bool is_timeout = cloud_recvreq_socket_server_ptr_->recv(global_request_msg_payload);
            if (is_timeout == true)
            {
                continue; // Retry to receive global request if cloud is still running
            } // End of (is_timeout == true)
            else
            {
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(global_request_msg_payload);
                assert(request_ptr != NULL);

                if (request_ptr->isGlobalRequest()) // Global requests
                {
                    NetworkAddr edge_cache_server_worker_recvrsp_dst_addr = request_ptr->getSourceAddr();
                    is_finish = processGlobalRequest_(request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release messages
                assert(request_ptr != NULL);
                delete request_ptr;
                request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if cloud is still running
                }
            } // End of (is_timeout == false)
        } // End of while loop

        // Wait cloud-to-edge propagation simulator
        pthread_returncode = pthread_join(cloud_toedge_propagation_simulator_thread, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join client-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
    }

    bool CloudWrapper::processGlobalRequest_(MessageBase* global_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        checkPointers_();

        const uint32_t cloud_idx = cloud_param_ptr_->getNodeIdx();

        bool is_finish = false;

        #ifdef DEBUG_CLOUD_WRAPPER
        struct timespec t0 = Util::getCurrentTimespec();
        #endif

        // Process global requests by RocksDB KVS
        MessageType global_request_message_type = global_request_ptr->getMessageType();
        Key tmp_key;
        Value tmp_value;
        MessageBase* global_response_ptr = NULL;
        switch (global_request_message_type)
        {
            case MessageType::kGlobalGetRequest:
            {
                const GlobalGetRequest* const global_get_request_ptr = static_cast<const GlobalGetRequest*>(global_request_ptr);
                tmp_key = global_get_request_ptr->getKey();

                // Get value from RocksDB KVS
                cloud_rocksdb_ptr_->get(tmp_key, tmp_value);

                // Prepare global get response message
                global_response_ptr = new GlobalGetResponse(tmp_key, tmp_value, cloud_idx, cloud_recvreq_source_addr_);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kGlobalPutRequest:
            {
                const GlobalPutRequest* const global_put_request_ptr = static_cast<const GlobalPutRequest*>(global_request_ptr);
                tmp_key = global_put_request_ptr->getKey();
                tmp_value = global_put_request_ptr->getValue();
                assert(tmp_value.isDeleted() == false);

                // Put value into RocksDB KVS
                cloud_rocksdb_ptr_->put(tmp_key, tmp_value);

                // Prepare global put response message
                global_response_ptr = new GlobalPutResponse(tmp_key, cloud_idx, cloud_recvreq_source_addr_);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                const GlobalDelRequest* const global_del_request_ptr = static_cast<const GlobalDelRequest*>(global_request_ptr);
                tmp_key = global_del_request_ptr->getKey();

                // Put value into RocksDB KVS
                cloud_rocksdb_ptr_->remove(tmp_key);

                // Prepare global del response message
                global_response_ptr = new GlobalDelResponse(tmp_key, cloud_idx, cloud_recvreq_source_addr_);
                assert(global_response_ptr != NULL);
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(global_request_message_type) << " for processGlobalRequest_()!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        #ifdef DEBUG_CLOUD_WRAPPER
        struct timespec t1 = Util::getCurrentTimespec();
        double delta_t = Util::getDeltaTimeUs(t1, t0);
        Util::dumpVariablesForDebug(instance_name_, 4, "keystr:", tmp_key.getKeystr().c_str(), "delta time (us) of rocksdb operation:", std::to_string(delta_t).c_str());
        #endif

        if (!is_finish) // Check is_finish
        {
            // Push the global response message into cloud-to-edge propagation simulator to edge cache server worker
            assert(global_response_ptr != NULL);
            assert(global_response_ptr->isGlobalResponse());
            bool is_successful = cloud_toedge_propagation_simulator_param_ptr_->push(global_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
            assert(is_successful);
        }

        // NOTE: global_response_ptr will be released by cloud-to-edge propagation simulator

        return is_finish;
    }

    void CloudWrapper::checkPointers_() const
    {
        assert(cloud_param_ptr_ != NULL);
        assert(cloud_rocksdb_ptr_ != NULL);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
        assert(cloud_toedge_propagation_simulator_param_ptr_ != NULL);

        return;
    }
}