#include "cloud/cloud_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "message/global_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CloudWrapper::kClassName("CloudWrapper");

    void* CloudWrapper::launchCloud(void* local_cloud_param_ptr)
    {
        CloudWrapper local_cloud((CloudParam*)local_cloud_param_ptr);
        local_cloud.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    CloudWrapper::CloudWrapper(CloudParam* local_cloud_param_ptr)
    {
        if (local_cloud_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_cloud_param_ptr is NULL!");
            exit(1);
        }
        local_cloud_param_ptr_ = local_cloud_param_ptr;
        assert(local_cloud_param_ptr_ != NULL);
        
        // Open local RocksDB KVS
        local_cloud_rocksdb_ptr_ = new RocksdbWrapper(Util::getLocalCloudRocksdbDirpath(local_cloud_param_ptr_->getGlobalCloudIdx()));
        assert(local_cloud_rocksdb_ptr_ != NULL);

        // Prepare a socket server on recvreq port
        uint16_t global_cloud_recvreq_port = Config::getGlobalCloudRecvreqPort();
        NetworkAddr host_addr(Util::ANY_IPSTR, global_cloud_recvreq_port);
        local_cloud_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);
    }
        
    CloudWrapper::~CloudWrapper()
    {
        // NOTE: no need to delete local_cloud_param_ptr, as it is maintained outside CloudWrapper

        // Close local RocksDB KVS
        assert(local_cloud_rocksdb_ptr_ != NULL);
        delete local_cloud_rocksdb_ptr_;
        local_cloud_rocksdb_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);
        delete local_cloud_recvreq_socket_server_ptr_;
        local_cloud_recvreq_socket_server_ptr_ = NULL;
    }

    void CloudWrapper::start()
    {
        assert(local_cloud_param_ptr_ != NULL);
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false; // Mark if local cloud node is finished

        while (local_cloud_param_ptr_->isCloudRunning()) // local_cloud_running_ is set as true by default
        {
            // Receive the global request message
            DynamicArray global_request_msg_payload;
            bool is_timeout = local_cloud_recvreq_socket_server_ptr_->recv(global_request_msg_payload);
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
                    is_finish = processGlobalRequest_(request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(kClassName, oss.str());
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
    }

    bool CloudWrapper::processGlobalRequest_(MessageBase* global_request_ptr)
    {
        assert(global_request_ptr != NULL && global_request_ptr->isGlobalRequest());
        assert(local_cloud_param_ptr_ != NULL);
        assert(local_cloud_rocksdb_ptr_ != NULL);
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;

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
                local_cloud_rocksdb_ptr_->get(tmp_key, tmp_value);

                // Prepare global get response message
                global_response_ptr = new GlobalGetResponse(tmp_key, tmp_value);
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
                local_cloud_rocksdb_ptr_->put(tmp_key, tmp_value);

                // Prepare global put response message
                global_response_ptr = new GlobalPutResponse(tmp_key);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                const GlobalDelRequest* const global_del_request_ptr = static_cast<const GlobalDelRequest*>(global_request_ptr);
                tmp_key = global_del_request_ptr->getKey();

                // Put value into RocksDB KVS
                local_cloud_rocksdb_ptr_->remove(tmp_key);

                // Prepare global del response message
                global_response_ptr = new GlobalDelResponse(tmp_key);
                assert(global_response_ptr != NULL);
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(global_request_message_type) << " for processGlobalRequest_()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        if (!is_finish) // Check is_finish
        {
            // Reply global response message to the edge node (the remote address set by the most recent recv)
            assert(global_response_ptr != NULL);
            assert(global_response_ptr->isGlobalResponse());
            DynamicArray global_response_msg_payload(global_response_ptr->getMsgPayloadSize());
            global_response_ptr->serialize(global_response_msg_payload);
            PropagationSimulator::propagateFromCloudToEdge();
            local_cloud_recvreq_socket_server_ptr_->send(global_response_msg_payload);
        }

        // Release global response message
        assert(global_response_ptr != NULL);
        delete global_response_ptr;
        global_response_ptr = NULL;

        return is_finish;
    }
}