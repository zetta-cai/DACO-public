/*
 * UdpSocketWrapper: encapsulate advanced operations (timeout-and-retry and UDP fragmentation) on UDP socket programming.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_SOCKET_WRAPPER_H
#define UDP_SOCKET_WRAPPER_H

#include <string>

#include "network/udp_socket_basic.h"

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
        static const bool IS_SOCKET_CLIENT_TIMEOUT;
        static const bool IS_SOCKET_SERVER_TIMEOUT;

        // TODO: === END HERE ===

        UdpSocketWrapper(const SocketRole& role, const std::string& ipstr, const uint16_t& udp_port);
        ~UdpSocketWrapper();

        // Note: UDP client always sends packets to the remote address set in the constructor, while UDP server sends the current UDP packet to the remote address set in the most recent successful recvfrom
        void send(const std::vector<char>& payload);
        void recv();
    private:
        static std::string kClassName;

        const SocketRole role_;
        const std::string udp_host_ipstr_; // only for UDP server
        const uint16_t udp_host_port_; // only for UDP server

        std::string udp_remote_ipstr_; // for UDP client (fixed) / server (changed)
        uint16_t udp_remote_port_; // for UDP client (fixed) / server (changed)
        bool is_receive_remote_address; // only for UDP server
    };
}

#endif

/*
#include <netinet/ether.h> // ether_header
#include <linux/ip.h> // iphdr
#include <linux/udp.h> // udphdr
#include <sys/ioctl.h> // ioctl
#include <netpacket/packet.h> // sockaddr_ll


void udpsendlarge_udpfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role);
void udpsendlarge_ipfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize);
void udpsendlarge(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
//bool udprecvlarge_udpfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role);
//bool udprecvlarge_ipfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize);
//bool udprecvlarge(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role);
bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr = NULL);
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr);*/