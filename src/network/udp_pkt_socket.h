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

#include "network/socket_result.h"

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
        UdpPktSocket(const bool& need_timeout, const std::string& host_ipstr, const uint16_t& host_port); // for UDP server
        ~UdpPktSocket();

        void sendto(const std::vector<char>& pkt_payload, const std::string& remote_ipstr, const uint16_t& remote_port);
        void recvfrom(SocketResult& socket_result); // Note: pass reference to avoid unnecessary memory copy
    private:
        static std::string kClassName;

        // UDP socket programming
        void createUdpsock();

        // Shared by UDP/TCP
        void setTimeout(); // for UDP client/server
        void enableReuseaddr(); // only for UDP server
        void bindSockaddr(const std::string& host_ipstr, const uint16_t& host_port); // only for UDP server

        const bool need_timeout_;
        int sockfd_;
    };
}

#endif
