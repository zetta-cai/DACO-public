/*
 * BeaconServerBase: listen to receive control requests issued by closest edge nodes; access content directory information for content discovery.
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

        // (2) Member variables

        std::string base_instance_name_;
        const EdgeWrapper* edge_wrapper_ptr_;

        UdpSocketWrapper* edge_beacon_server_recvreq_socket_server_ptr_;
    };
}

#endif