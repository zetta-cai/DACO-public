/*
 * EdgeWrapperBase: the base class of edge node to process data/control requests.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_BASE_H
#define EDGE_WRAPPER_BASE_H

#include <string>

#include "cache/cache_wrapper_base.h"
#include "edge/basic_edge_wrapper.h"
#include "edge/edge_param.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class EdgeWrapperBase
    {
    public:
        static EdgeWrapperBase* getEdgeWrapper(const std::string& cache_name, EdgeParam* edge_param_ptr);

        EdgeWrapperBase(const std::string& cache_name, EdgeParam* edge_param_ptr);
        virtual ~EdgeWrapperBase();

        void start();
    private:
        static const std::string kClassName;

        // Return is_finish
        bool processDataRequest_(MessageBase* request_ptr);
        bool processLocalGetRequest_(MessageBase* request_ptr);
        bool processLocalWriteRequest_(MessageBase* request_ptr); // For put/del
        bool processRedirectedRequest_(MessageBase* request_ptr);

        // Return is_finish
        virtual bool processControlRequest_(MessageBase* request_ptr) = 0;

        // Return is_finish
        bool blockForInvalidation_(const Key& key);
        bool fetchDataFromCloud_(const Key& key, Value& value);
        bool writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type);

        void triggerIndependentAdmission_(const Key& key, const Value& value);

        const std::string cache_name_;
        EdgeParam* edge_param_ptr_;
        CacheWrapperBase* edge_cache_ptr_;
        UdpSocketWrapper* edge_recvreq_socket_server_ptr_;
        UdpSocketWrapper* edge_sendreq_tocloud_socket_client_ptr_;
    };
}

#endif