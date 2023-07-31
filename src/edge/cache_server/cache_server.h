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
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class CacheServerWorkerParam;

    class CacheServer
    {
    public:
        CacheServer(EdgeWrapper* edge_wrapper_ptr);
        ~CacheServer();

        void start();

        EdgeWrapper* getEdgeWrapperPtr() const;
        NetworkAddr getEdgeCacheServerRecvreqSourceAddr() const;
    private:
        static const std::string kClassName;

        void receiveRequestsAndPartition_();
        void partitionRequest_(MessageBase* data_requeset_ptr);

        void checkPointers_() const;

        // Const shared variable
        std::string instance_name_;
        EdgeWrapper* edge_wrapper_ptr_;
        HashWrapperBase* hash_wrapper_ptr_;

        // Non-const individual variable
        std::vector<CacheServerWorkerParam> cache_server_worker_params_; // Each cache server thread has a unique param

        // For receiving local requests
        NetworkAddr edge_cache_server_recvreq_source_addr_; // The same as that used by clients or neighbors to send local/redirected requests (const shared variable)
        UdpMsgSocketServer* edge_cache_server_recvreq_socket_server_ptr_; // Used by cache server to receive local requests from clients and redirected requests from neighbors (non-const individual variable)
    };
}

#endif