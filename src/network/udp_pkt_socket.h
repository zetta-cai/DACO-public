/*
 * UdpSocketBasic: encapsulate basic operations (send/receive payload of a single UDP packet) on UDP socket programming.
 *
 * Note: UdpSocketBasic is orthogonal with UDP fragmentation (see UdpSocketWrapper).
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef UDP_PKT_SOCKET_H
#define UDP_PKT_SOCKET_H

#include <string>
#include <vector>
#include <errno.h> // errno
#include <sys/socket.h> // socket API
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // htons ntohs inet_ntop inet_pton

#include "network/network_addr.h"

namespace covered
{
    class UdpPktSocket
    {
    public:
        // Settings on UDP socket fd (only used in UdpSocketBasic)
        static const uint32_t SOCKET_TIMEOUT_SECONDS;
        static const uint32_t SOCKET_TIMEOUT_USECONDS;
        static const uint32_t UDP_DEFAULT_RCVBUFSIZE;
        //static const uint32_t UDP_LARGE_RCVBUFSIZE;

        UdpPktSocket(const bool& need_timeout); // for UDP client
        UdpPktSocket(const bool& need_timeout, const NetworkAddr& host_addr); // for UDP server
        ~UdpPktSocket();

        // Note: pass reference of pkt_payload to avoid unnecessary memory copy
        void sendto(const std::vector<char>& pkt_payload, const NetworkAddr& remote_addr);
        bool recvfrom(std::vector<char>& pkt_payload, NetworkAddr& remote_addr); // Return timeout flag
    private:
        static std::string kClassName;

        // UDP socket programming
        void createUdpsock_();

        // Shared by UDP/TCP
        void setTimeout_(); // for UDP client/server
        void enableReuseaddr_(); // only for UDP server
        void bindSockaddr_(const NetworkAddr& host_addr); // only for UDP server

        const bool need_timeout_;
        int sockfd_;
    };
}

#endif
