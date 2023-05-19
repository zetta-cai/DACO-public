#include "cloud/cloud_wrapper.h"

#include <assert.h>

#include "common/util.h"
#include "message/global_message.h"
#include "network/network_addr.h"

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

    CloudWrapper::CloudWrapper(EdgeParam* local_cloud_param_ptr)
    {
        if (local_cloud_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_cloud_param_ptr is NULL!");
            exit(1);
        }
        local_cloud_param_ptr_ = local_cloud_param_ptr;
        assert(local_cloud_param_ptr_ != NULL);
        
        // TODO: Open local RocksDB KVS

        // Prepare a socket server on recvreq port
        uint16_t global_cloud_recvreq_port = Config::getGlobalCloudRecvreqPort();
        NetworkAddr host_addr(Util::ANY_IPSTR, global_cloud_recvreq_port);
        local_cloud_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);
    }
        
    CloudWrapper::~CloudWrapper()
    {
        // NOTE: no need to delete local_cloud_param_ptr, as it is maintained outside CloudWrapper

        // TODO: Close local RocksDB KVS

        // Free the socket server on recvreq port
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);
        delete local_cloud_recvreq_socket_server_ptr_;
        local_cloud_recvreq_socket_server_ptr_ = NULL;
    }

    void CloudWrapper::start()
    {
        assert(local_cloud_param_ptr != NULL);
        assert(local_cloud_recvreq_socket_server_ptr_ != NULL);

        while (local_cloud_param_ptr->isCloudRunning()) // local_cloud_running_ is set as true by default
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
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(request_msg_payload);
                assert(request_ptr != NULL);

                if (request_ptr->isGlobalRequest()) // Global requests
                {
                    processGlobalRequest_(request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(kClassName, oss.str());
                    exit(1);
                }

                // Free messages
                assert(request_ptr != NULL);
                delete request_ptr;
                request_ptr = NULL;
            } // End of (is_timeout == false)
        } // End of while loop
    }

    void CloudWrapper::processGlobalRequest_(MessageBase* global_request_ptr)
    {
        assert(global_request_ptr != NULL);

        // TODO: Process global requests by RocksDB KVS
        // TODO: Reply global responses to the edge node
    }
}