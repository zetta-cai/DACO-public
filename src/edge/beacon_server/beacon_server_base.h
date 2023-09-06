/*
 * BeaconServerBase: listen to receive control requests issued by closest edge nodes; access content directory information for content discovery.
 *
 * A. Involved messages triggered by local get requests
 * (1) Receive/issue directory lookup requests/reponses
 * (2) Issue/receive finish block requests/responses
 * (3) Receive/issue directory update requests/reponses
 * 
 * B. Involved messages triggered by local put/del requests
 * (1) Receive/issue acquire writelock requests/responses
 * (2) Issue/receive invalidation requests/responses
 * (3) Receive/issue finish write requests/responses
 * (4) Issue/receive finish block requests/responses
 * (5) Receive/issue directory update requests/reponses
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef BEACON_SERVER_BASE_H
#define BEACON_SERVER_BASE_H

//#define DEBUG_BEACON_SERVER

#include <string>

#include "edge/edge_wrapper.h"
#include "message/message_base.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class BeaconServerBase
    {
    public:
        static BeaconServerBase* getBeaconServerByCacheName(EdgeWrapper* edge_wrapper_ptr);

        BeaconServerBase(EdgeWrapper* edge_wrapper_ptr);
        virtual ~BeaconServerBase();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);

        // (1) Access content directory information

        // Return if edge node is finished
        bool processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) const;
        // NOTE: as a directory update has limited impact on cache size, we do NOT check capacity and trigger eviction for performance (capacity is only checked for cache admission and value updates)
        virtual void processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_source_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0;
        virtual MessageBase* getRspToLookupLocalDirectory_(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const EventList& event_list, const bool& skip_propagation_latency) const = 0;

        bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr); // Return if edge node is finished
        virtual bool processReqToUpdateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info) = 0; // Return if key is being written

        // (2) Process writes and unblock for MSI protocol

        // Return if edge node is finished
        virtual bool processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) = 0;
        virtual bool processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) = 0;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) = 0;

        // Member varaibles

        // Const variable
        std::string base_instance_name_;
    protected:
        // (4) Utility functions
        void checkPointers_() const;

        // Member variables

        // Const variable
        const EdgeWrapper* edge_wrapper_ptr_;

        // For receiving control requests
        NetworkAddr edge_beacon_server_recvreq_source_addr_; // The same as cache server worker to send control requests (const individual variable)
        UdpMsgSocketServer* edge_beacon_server_recvreq_socket_server_ptr_; // Used by beacon server to receive control requests from cache server workers (non-const individual variable)

        // For receiving control responses (e.g., InvalidationResponse and FinishBlockResponse)
        NetworkAddr edge_beacon_server_recvrsp_source_addr_; // Used by invalidation server or cache server worker to send back control responses (const individual variable)
        UdpMsgSocketServer* edge_beacon_server_recvrsp_socket_server_ptr_; // Used by beacon server to receive control requests from invalidation server or cache server worker (non-const individual variable)
    };
}

#endif