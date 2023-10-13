/*
 * DataServer: data server thread launched by cloud to process global requests and reply global responses.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef DATA_SERVER_H
#define DATA_SERVER_H

//#define DEBUG_DATA_SERVER

#include <string>

#include "cloud/cloud_wrapper.h"
#include "message/message_base.h"
#include "network/network_addr.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class DataServer
    {
    public:
        static void* launchDataServer(void* cloud_wrapper_ptr);
        
        DataServer(CloudWrapper* cloud_wrapper_ptr);
        ~DataServer();

        void start();
    private:
        static const std::string kClassName;

        bool processGlobalRequest_(MessageBase* request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);

        void checkPointers_() const;

        std::string instance_name_;
        CloudWrapper* cloud_wrapper_ptr_;

        // For receiving global requests
        NetworkAddr cloud_recvreq_source_addr_; // The same as that used by edge cache server worker to send global requests (const individual variable)
        UdpMsgSocketServer* cloud_recvreq_socket_server_ptr_; // Used by cloud to receive global requests from edge cache server worker (non-const individual variable)
    };
}

#endif