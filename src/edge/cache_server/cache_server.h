/*
 * CacheServer: listen to receive local/redirected requests issued by clients/neighbors; launch multiple cache server worker threads to process received requests in parallel.
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_H
#define CACHE_SERVER_H

#include <string>
#include <vector>

namespace covered
{
    class CacheServer;
}

#include "concurrency/rwlock.h"
#include "edge/cache_server/cache_server_processor_param.h"
#include "edge/cache_server/cache_server_worker_param.h"
#include "edge/edge_wrapper.h"
#include "hash/hash_wrapper_base.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class CacheServer
    {
    public:
        CacheServer(EdgeWrapper* edge_wrapper_ptr);
        ~CacheServer();

        void start();

        EdgeWrapper* getEdgeWrapperPtr() const;
        NetworkAddr getEdgeCacheServerRecvreqSourceAddr() const;

        // Return if edge node is finished
        bool admitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background = false) const; // Admit directory info in remote beacon node
    private:
        static const std::string kClassName;

        void receiveRequestsAndPartition_();
        void partitionRequest_(MessageBase* data_requeset_ptr);

        // Admit content directory information (invoked by edge cache server worker or placement processor)

        MessageBase* getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const;
        void processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, const bool& is_background) const;

        void checkPointers_() const;

        // Const shared variable
        std::string instance_name_;
        EdgeWrapper* edge_wrapper_ptr_;
        HashWrapperBase* hash_wrapper_ptr_;

        // Non-const individual variable
        std::vector<CacheServerWorkerParam> cache_server_worker_params_; // Each cache server thread has a unique param
        CacheServerProcessorParam cache_server_victim_fetch_processor_param_; // Only one cache server victim fetch processor thread
        CacheServerProcessorParam cache_server_placement_processor_param_; // Only one cache server placement processor thread

        // For receiving local requests
        NetworkAddr edge_cache_server_recvreq_source_addr_; // The same as that used by clients or neighbors to send local/redirected requests (const shared variable)
        UdpMsgSocketServer* edge_cache_server_recvreq_socket_server_ptr_; // Used by cache server to receive local requests from clients and redirected requests from neighbors (non-const individual variable)
    };
}

#endif