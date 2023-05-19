/*
 * EdgeWrapper: the edge node to receive data/control requests and send responses.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <string>

#include "edge/edge_param.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class EdgeWrapper
    {
    public:
        static void* launchEdge(void* local_edge_param_ptr);

        EdgeWrapper(EdgeParam* local_edge_param_ptr);
        ~EdgeWrapper();

        void start();
    private:
        static const std::string kClassName;

        // Return is_finish
        bool processDataRequest_(MessageBase* request_ptr);
        bool processLocalGetRequest_(MessageBase* request_ptr);
        bool processLocalWriteRequest_(MessageBase* request_ptr); // For put/del
        bool processRedirectedRequest_(MessageBase* request_ptr);

        // Return is_finish
        bool processControlRequest_(MessageBase* request_ptr);

        // Return is_finish
        bool blockForInvalidation_(const Key& key);
        bool fetchDataFromCloud_(const Key& key, Value& value);

        void triggerIndependentAdmission_(const Key& key, const Value& value);

        EdgeParam* local_edge_param_ptr_;
        CacheWrapperBase* local_edge_cache_ptr_;
        UdpSocketWrapper* local_edge_recvreq_socket_server_ptr_;
        UdpSocketWrapper* local_edge_sendreq_tocloud_socket_client_ptr_;
    };
}

#endif