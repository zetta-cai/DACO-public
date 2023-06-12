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

    CloudWrapper::CloudWrapper(const std::string& cloud_storage, CloudParam* cloud_param_ptr)
    {
        if (cloud_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "cloud_param_ptr is NULL!");
            exit(1);
        }

        // Different different clouds if any
        std::ostringstream oss;
        oss << kClassName << " " << cloud_param_ptr->getCloudIdx();
        instance_name_ = oss.str();

        cloud_param_ptr_ = cloud_param_ptr;
        assert(cloud_param_ptr_ != NULL);
        
        // Open local RocksDB KVS
        cloud_rocksdb_ptr_ = new RocksdbWrapper(cloud_storage, Util::getCloudRocksdbDirpath(cloud_param_ptr_->getCloudIdx()), cloud_param_ptr);
        assert(cloud_rocksdb_ptr_ != NULL);

        // Prepare a socket server on recvreq port
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(cloud_param_ptr_->getCloudIdx());
        NetworkAddr host_addr(Util::ANY_IPSTR, cloud_recvreq_port);
        cloud_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
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
    }

    void CloudWrapper::start()
    {
        assert(cloud_param_ptr_ != NULL);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false; // Mark if local cloud node is finished

        while (cloud_param_ptr_->isCloudRunning()) // cloud_running_ is set as true by default
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
                    is_finish = processGlobalRequest_(request_ptr);
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
    }

    bool CloudWrapper::processGlobalRequest_(MessageBase* global_request_ptr)
    {
        assert(global_request_ptr != NULL && global_request_ptr->isGlobalRequest());
        assert(cloud_param_ptr_ != NULL);
        assert(cloud_rocksdb_ptr_ != NULL);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);

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
                cloud_rocksdb_ptr_->get(tmp_key, tmp_value);

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
                cloud_rocksdb_ptr_->put(tmp_key, tmp_value);

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
                cloud_rocksdb_ptr_->remove(tmp_key);

                // Prepare global del response message
                global_response_ptr = new GlobalDelResponse(tmp_key);
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

        if (!is_finish) // Check is_finish
        {
            // Reply global response message to the edge node (the remote address set by the most recent recv)
            assert(global_response_ptr != NULL);
            assert(global_response_ptr->isGlobalResponse());
            DynamicArray global_response_msg_payload(global_response_ptr->getMsgPayloadSize());
            global_response_ptr->serialize(global_response_msg_payload);
            PropagationSimulator::propagateFromCloudToEdge();
            cloud_recvreq_socket_server_ptr_->send(global_response_msg_payload);
        }

        // Release global response message
        assert(global_response_ptr != NULL);
        delete global_response_ptr;
        global_response_ptr = NULL;

        return is_finish;
    }
}