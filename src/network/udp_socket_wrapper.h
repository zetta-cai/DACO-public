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

        UdpSocketWrapper(const SocketRole& role, const std::string& ipstr, const uint16_t& port);
        ~UdpSocketWrapper();

        // Note: UDP client always sends message payload to the remote address set in the constructor
        // Note: UDP server sends message payload to the remote address set in the most recent successful recv (i.e., receive all fragment payloads of a message payload)
        void send(const std::vector<char>& msg_payload);
        void recv();
    private:
        static std::string kClassName;

        const SocketRole role_;
        const std::string host_ipstr_; // only for UDP server
        const uint16_t host_port_; // only for UDP server

        std::string remote_ipstr_; // for UDP client (fixed) / server (changed)
        uint16_t remote_port_; // for UDP client (fixed) / server (changed)
        bool is_receive_remote_address; // only for UDP server
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


void udpsendlarge_udpfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role);
void udpsendlarge_ipfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize);
void udpsendlarge(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
//bool udprecvlarge_udpfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role);
//bool udprecvlarge_ipfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize);
//bool udprecvlarge(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role);
bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr = NULL);
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr);*/