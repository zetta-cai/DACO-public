/*
 * CacheServer: listen to receive local/redirected requests issued by clients/neighbors; launch multiple cache server worker threads to process received requests in parallel.
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_H
#define CACHE_SERVER_H

#include <string>
#include <vector>

#include "edge/cache_server/cache_server_worker_param.h"
#include "edge/edge_wrapper.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    class CacheServer
    {
    public:
        CacheServer(EdgeWrapper* edge_wrapper_ptr);
        ~CacheServer();

        void start();
    private:
        static const std::string kClassName;

        void receiveRequestsAndPartition_();
        void partitionRequest_(MessageBase* data_requeset_ptr);

        void checkPointers_() const;

        // Const variable
        std::string instance_name_;
        EdgeWrapper* edge_wrapper_ptr_;
        HashWrapperBase* hash_wrapper_ptr_;

        // Non-const individual variable
        UdpSocketWrapper* edge_cache_server_recvreq_socket_server_ptr_; // Only used by cache server
        std::vector<CacheServerWorkerParam> cache_server_worker_params_; // Each cache server thread has a unique param
    };
}

#endif