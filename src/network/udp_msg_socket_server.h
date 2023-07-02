/*
 * UdpMsgSocketServer: encapsulate advanced operations (UDP fragmentation of message payload) on UDP socket programming to receive messages.
 *
 * NOTE: message payload may be splited into multiple UDP fragments (i.e., multiple fragment payloads), where each fragment payload + fragment header form the corresponding UDP packet payload.
 * 
 * NOTE: UdpMsgSocketServer is only used by processing threads of each node (Client/Edge/CloudWrapper) to receive messages with a returned remote address.
 * 
 * By Siyuan Sheng (2023.07.02).
 */

#ifndef UDP_MSG_SOCKET_SERVER_H
#define UDP_MSG_SOCKET_SERVER_H

#include <string>

#include "common/dynamic_array.h"
#include "network/msg_frag_stats.h"
#include "network/network_addr.h"
#include "network/udp_pkt_socket.h"

namespace covered
{
    class UdpMsgSocketServer
    {
    public:
        // Whether to enable timeout for UDP server (only used in UdpMsgSocketServer)
        static const bool IS_SOCKET_SERVER_TIMEOUT;

        UdpMsgSocketServer(const NetworkAddr& host_addr);
        ~UdpMsgSocketServer();

        // Note: pass reference of pkt_payload to avoid unnecessary memory copy
        bool recv(DynamicArray& msg_payload, NetworkAddr& network_addr);
    private:
        static const std::string kClassName;

        UdpPktSocket* pkt_socket_ptr_; // recv payload of each single UDP packet
        MsgFragStats msg_frag_stats_; // reconstruct each message based on received fragments
    };
}

#endif