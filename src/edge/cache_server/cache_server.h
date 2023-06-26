/*
 * CacheServer: listen to receive local requests issued by clients; launch multiple cache server worker threads to process received requests in parallel.
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_H
#define CACHE_SERVER_H

#include <string>

#include "edge/edge_wrapper.h"
#include "network/udp_socket_wrapper.h"

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

        static void* launchCacheServerWorker_(void* cache_server_worker_param_ptr);

        // Const variable
        std::string instance_name_;
        EdgeWrapper* edge_wrapper_ptr_;

        // Non-const individual variable
        UdpSocketWrapper* edge_cache_server_recvreq_socket_server_ptr_;
    };
}

#endif