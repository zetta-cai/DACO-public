/*
 * EdgeWrapperBase: the base class of edge node to process data/control requests.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_BASE_H
#define EDGE_WRAPPER_BASE_H

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "edge/edge_param.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class EdgeWrapperBase
    {
    public:
        static EdgeWrapperBase* getEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);

        EdgeWrapperBase(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~EdgeWrapperBase();

        void start();
    private:
        static const std::string kClassName;

        // (1) Data requests
    
        // Return if edge node is finished
        bool processDataRequest_(MessageBase* data_request_ptr);
        bool processLocalGetRequest_(MessageBase* local_request_ptr);
        bool processLocalWriteRequest_(MessageBase* local_request_ptr); // For put/del
        bool processRedirectedRequest_(MessageBase* redirected_request_ptr);
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr) = 0;

        // Return if edge node is finished
        bool blockForInvalidation_(const Key& key);
        bool fetchDataFromCloud_(const Key& key, Value& value);
        bool writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type);

        virtual void triggerIndependentAdmission_(const Key& key, const Value& value) = 0;

        // (2) Control requests

        // Return if edge node is finished
        bool processControlRequest_(MessageBase* control_request_ptr);
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr) = 0;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) = 0;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) = 0;

        std::string base_instance_name_;
    protected:
        const std::string cache_name_;
        EdgeParam* edge_param_ptr_;

        CacheWrapperBase* edge_cache_ptr_;
        CooperationWrapperBase* cooperation_wrapper_ptr_;

        UdpSocketWrapper* edge_recvreq_socket_server_ptr_;
        UdpSocketWrapper* edge_sendreq_tocloud_socket_client_ptr_;
    };
}

#endif