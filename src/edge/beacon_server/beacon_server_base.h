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

#include <string>

#include "edge/edge_wrapper.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class BeaconServerBase
    {
    public:
        static BeaconServerBase* getBeaconServer(EdgeWrapper* edge_wrapper_ptr);

        BeaconServerBase(EdgeWrapper* edge_wrapper_ptr);
        ~BeaconServerBase();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr);

        // (1) Access content directory information

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const = 0;
        // NOTE: as a directory update has limited impact on cache size, we do NOT check capacity and trigger eviction for performance (capacity is only checked for cache admission and value updates)
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) = 0;

        // (2) Process writes and unblock for MSI protocol

        // Return if edge node is finished
        virtual bool processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) = 0;
        bool notifyEdgesToFinishBlock_(const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges) const;
        virtual void sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const = 0;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) = 0;

        // Member varaibles

        // Const variable
        std::string base_instance_name_;
    protected:
        // (2) Unblock for MSI protocol

        // Return if edge node is finished
        bool tryToNotifyEdgesFromBlocklist_(const Key& key) const;

        // Member variables

        // Const variable
        const EdgeWrapper* edge_wrapper_ptr_;

        // Non-const individual variable
        UdpSocketWrapper* edge_beacon_server_recvreq_socket_server_ptr_;
        UdpSocketWrapper* edge_beacon_server_sendreq_toblocked_socket_client_ptr_;
    };
}

#endif