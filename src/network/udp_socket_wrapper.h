/*
 * UdpSocketWrapper: encapsulate advanced operations (UDP fragmentation of message payload) on UDP socket programming.
 *
 * NOTE: message payload may be splited into multiple UDP fragments (i.e., multiple fragment payloads).
 * NOTE: each fragment payload + fragment header form the corresponding packet payload.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_SOCKET_WRAPPER_H
#define UDP_SOCKET_WRAPPER_H

#include <string>

#include "network/network_addr.h"
#include "network/udp_pkt_socket.h"

namespace covered
{
    enum SocketRole
    {
        kSocketClient = 0,
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
        void send(const std::vector<char>& msg_payload);
        bool recv(std::vector<char>& msg_payload);
    private:
        static std::string kClassName;

        const SocketRole role_;
        const NetworkAddr host_addr_; // only for UDP server

        // Note: remote address of UDP client is always that set in the constructor
        // Note: remote address of UDP server is that set in the most recent successful recv (i.e., receive all fragment payloads of a message payload)
        NetworkAddr remote_addr_; // for UDP client (fixed) / server (changed)
        UdpPktSocket* pkt_socket_ptr_;
    };
}

#endif

/*
#include <netinet/ether.h> // ether_header
#include <linux/ip.h> // iphdr
#include <linux/udp.h> // udphdr
#include <sys/ioctl.h> // ioctl
#include <netpacket/packet.h> // sockaddr_ll


bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role);
bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr = NULL);
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr);*/