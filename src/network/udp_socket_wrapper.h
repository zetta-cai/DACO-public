/*
 * UdpSocketWrapper: encapsulate operations on UDP socket programming.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef UDP_SOCKET_WRAPPER_H
#define UDP_SOCKET_WRAPPER_H

#include <string>
#include <vector>
#include <errno.h> // errno
#include <sys/socket.h> // socket API
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // htons htonl inet_ntop inet_pton

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
        static const uint32_t SOCKET_TIMEOUT_SECONDS;
        static const uint32_t SOCKET_TIMEOUT_USECONDS;
        static const uint32_t UDP_PAYLOAD_MAXSIZE;
        static const uint32_t UDP_DEFAULT_RCVBUFSIZE;
        //static const uint32_t UDP_LARGE_RCVBUFSIZE;

        UdpSocketWrapper(const SocketRole& role, const bool& need_timeout, const std::string& ipstr, const uint16_t& port);

        void sendto(const std::vector<char>& payload);
        // TODO: recvfrom
    privae:
        static std::string kClassName;

        // UDP socket programming
        void createUdpsock();

        // Shared by UDP/TCP
        void setTimeout(); // for UDP client/server
        void enableReuseaddr(); // only for UDP server
        void bindSockaddr(); // only for UDP server

        const SocketRole role_;
        const bool need_timeout_;
        const std::string udp_host_ipstr_; // only for UDP server
        const uint16_t udp_host_port_; // only for UDP server

        std::string udp_remote_ipstr_; // for UDP client (fixed) / server (changed)
        uint16_t udp_remote_port_; // for UDP client (fixed) / server (changed)
        bool is_receive_remote_address; // only for UDP server
        int sockfd_;
    };
}

/*
#include <netinet/ether.h> // ether_header
#include <linux/ip.h> // iphdr
#include <linux/udp.h> // udphdr
#include <sys/ioctl.h> // ioctl
#include <netpacket/packet.h> // sockaddr_ll



// udp
bool udprecvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role = "sockethelper.udprecvfrom");

void udpsendlarge_udpfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role);
void udpsendlarge_ipfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize);
void udpsendlarge(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
//bool udprecvlarge_udpfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role);
//bool udprecvlarge_ipfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize);
//bool udprecvlarge(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role);
bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr = NULL);
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr);*/

#endif
