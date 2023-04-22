/*
 * SocketWrapper: encapsulate operations on socket programming.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <string>
#include <errno.h> // errno
#include <sys/socket.h> // socket API
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // htons htonl

namespace covered
{
    enum SocketRole
    {
        kSocketClient = 0,
        kSocketServer
    };

    class SocketWrapper
    {
    public:
        static const uint32_t SOCKET_TIMEOUT_SECONDS;
        static const uint32_t SOCKET_TIMEOUT_USECONDS;
        static const uint32_t UDP_PAYLOAD_MAXSIZE;
        static const uint32_t UDP_DEFAULT_RCVBUFSIZE;
        //static const uint32_t UDP_LARGE_RCVBUFSIZE;

        SocketWrapper(const SocketRole& role, const bool& need_timeout, const uint16_t& port = 0);
    privae:
        static std::string kClassName;

        // UDP socket programming
        void createUdpsock();

        // Shared by UDP/TCP
        void setTimeout(); // client/server
        void enableReuseaddr(); // only for server
        void bindSockaddr(); // only for server

        const SocketRole role_;
        const bool need_timeout_;
        const uint16_t port_;
        int sockfd_;
    };
}

/*#include <net/if.h> // struct ifreq; ifname -> ifidx
#include <netinet/ether.h> // ether_header
#include <linux/ip.h> // iphdr
#include <linux/udp.h> // udphdr
#include <sys/ioctl.h> // ioctl
#include <netpacket/packet.h> // sockaddr_ll
#include <linux/filter.h> // sock filter



// udp
void udpsendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role = "sockethelper.udpsendto");
bool udprecvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role = "sockethelper.udprecvfrom");

void udpsendlarge_udpfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role);
void udpsendlarge_ipfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize);
void udpsendlarge(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
//bool udprecvlarge_udpfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role);
//bool udprecvlarge_ipfrag(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize);
//bool udprecvlarge(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role, size_t frag_hdrsize, size_t frag_maxsize);
//bool udprecvlarge_multisrc_udpfrag(int sockfd, void *bufs, size_t bufnum, size_t len, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, int *recvsizes, int& recvnum, const char* role, size_t srcnum_off, size_t srcnum_len, bool srcnum_conversion, size_t srcid_off, size_t srcid_len, bool srcid_conversion);
//bool udprecvlarge_multisrc_ipfrag(int sockfd, void *bufs, size_t bufnum, size_t len, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, int *recvsizes, int& recvnum, const char* role, size_t frag_hdrsize, size_t srcnum_off, size_t srcnum_len, bool srcnum_conversion, size_t srcid_off, size_t srcid_len, bool srcid_conversion);
//bool udprecvlarge_multisrc(int sockfd, void *bufs, size_t bufnum, size_t len, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, int *recvsizes, int& recvnum, const char* role, size_t frag_hdrsize, size_t frag_maxsize, size_t srcnum_off, size_t srcnum_len, bool srcnum_conversion, size_t srcid_off, size_t srcid_len, bool srcid_conversion);
bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role);
bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr = NULL);
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr);
//bool udprecvlarge_multisrc_udpfrag(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());
bool udprecvlarge_multisrc_ipfrag(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());
bool udprecvlarge_multisrc(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, size_t frag_maxsize, int fragtype, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());
//bool udprecvlarge_multisrc_udpfrag_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());
bool udprecvlarge_multisrc_ipfrag_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());
bool udprecvlarge_multisrc_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, size_t frag_maxsize, int fragtype, bool isfilter=false, optype_t optype=0, netreach_key_t targetkey=netreach_key_t());*/

#endif
