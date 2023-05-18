/*
 * UdpSocketWrapper: encapsulate advanced operations (UDP fragmentation of message payload) on UDP socket programming.
 *
 * NOTE: message payload may be splited into multiple UDP fragments (i.e., multiple fragment payloads).
 * NOTE: each fragment payload + fragment header form the corresponding UDP packet payload.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_SOCKET_WRAPPER_H
#define UDP_SOCKET_WRAPPER_H

#include <string>

#include "common/dynamic_array.h"
#include "network/msg_frag_stats.h"
#include "network/network_addr.h"
#include "network/udp_pkt_socket.h"

namespace covered
{
    enum SocketRole
    {
        kSocketClient = 1,
        kSocketServer
    };

    class UdpSocketWrapper
    {
    public:
        // Whether to enable timeout for UDP client/server (only used in UdpSocketWrapper)
        static const bool IS_SOCKET_CLIENT_TIMEOUT;
        static const bool IS_SOCKET_SERVER_TIMEOUT;

        UdpSocketWrapper(const SocketRole& role, const NetworkAddr& addr);
        ~UdpSocketWrapper();

        // Note: pass reference of pkt_payload to avoid unnecessary memory copy
        void send(const DynamicArray& msg_payload);
        bool recv(DynamicArray& msg_payload);
    private:
        static const std::string kClassName;

        const SocketRole role_;
        const NetworkAddr host_addr_; // only for UDP server

        // Note: remote address of UDP client is always that set in the constructor
        // Note: remote address of UDP server is that set in the most recent successful recv (i.e., receive all fragment payloads of a message payload)
        NetworkAddr remote_addr_; // for UDP client (fixed) / server (changed)

        UdpPktSocket* pkt_socket_ptr_; // send/recv payload of each single UDP packet
        MsgFragStats msg_frag_stats_; // reconstruct each message based on received fragments
        uint32_t msg_seqnum_;
    };
}

#endif