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

#include "core/popularity/fast_path_hint.h"
#include "edge/edge_wrapper_base.h"
#include "edge/edge_custom_func_param_base.h"
#include "message/message_base.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class BeaconServerBase
    {
    public:
        static void* launchBeaconServer(void* edge_wrapper_ptr);
        static BeaconServerBase* getBeaconServerByCacheName(EdgeWrapperBase* edge_wrapper_ptr);

        BeaconServerBase(EdgeWrapperBase* edge_wrapper_ptr);
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
        virtual bool processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const = 0;
        virtual MessageBase* getRspToLookupLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const = 0;

        bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr); // Return if edge node is finished
        virtual bool processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, bool& is_neighbor_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list) = 0; // Return if key is being written
        virtual MessageBase* getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_neighbor_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const = 0;

        // (2) Process writes and unblock for MSI protocol

        // Return if edge node is finished
        bool processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);
        virtual bool processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list) = 0; // Return if edge node is finished
        virtual MessageBase* getRspToAcquireLocalWritelock_(MessageBase* control_request_ptr, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const = 0;

        // Return if edge node is finished
        bool processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);
        virtual bool processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list) = 0; // Return if edge node is finished
        virtual MessageBase* getRspToReleaseLocalWritelock_(MessageBase* control_request_ptr, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const = 0;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) = 0;

        // (4) Embed background events and bandwidth usage

        void embedBackgroundCounterIfNotEmpty_(BandwidthUsage& bandwidth_usage, EventList& event_list) const;

        // (5) Cache-method-specific custom functions

        virtual void customFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) = 0;

        // Member varaibles

        // Const variable
        std::string base_instance_name_;
    protected:
        // (6) Utility functions
        void checkPointers_() const;

        // Member variables

        // Const variable
        mutable EdgeWrapperBase* edge_wrapper_ptr_; // Mutable to update and load&reset beacon background counter

        // For receiving control requests
        NetworkAddr edge_beacon_server_recvreq_source_addr_; // The same as cache server worker to send control requests (const individual variable)
        UdpMsgSocketServer* edge_beacon_server_recvreq_socket_server_ptr_; // Used by beacon server to receive control requests from cache server workers (non-const individual variable)

        // For receiving control responses (e.g., InvalidationResponse and FinishBlockResponse)
        NetworkAddr edge_beacon_server_recvrsp_source_addr_; // Used by invalidation server or cache server worker to send back control responses (const individual variable)
        UdpMsgSocketServer* edge_beacon_server_recvrsp_socket_server_ptr_; // Used by beacon server to receive control requests from invalidation server or cache server worker (non-const individual variable)
    };
}

#endif