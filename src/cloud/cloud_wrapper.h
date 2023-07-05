/*
 * CloudWrapper: the cloud to receive global requests and send responses.
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#ifndef CLOUD_WRAPPER_H
#define CLOUD_WRAPPER_H

#include <string>

#include "cloud/cloud_param.h"
#include "cloud/rocksdb_wrapper.h"
#include "message/message_base.h"

namespace covered
{
    class CloudWrapper
    {
    public:
        static void* launchCloud(void* cloud_param_ptr);
        
        CloudWrapper(const std::string& cloud_storage, CloudParam* cloud_param_ptr);
        ~CloudWrapper();

        void start();
    private:
        static const std::string kClassName;

        bool processGlobalRequest_(MessageBase* request_ptr);

        std::string instance_name_;
        CloudParam* cloud_param_ptr_;
        RocksdbWrapper* cloud_rocksdb_ptr_;
        UdpSocketWrapper* cloud_recvreq_socket_server_ptr_;
    };
}

#endif