#include "network/udp_socket_basic.h"

#include <sstream>
#include <cstring> // memset

#include "common/util.h"

namespace covered
{
    const uint32_t UdpPktSocket::SOCKET_TIMEOUT_SECONDS = 5; // 5s
    const uint32_t UdpPktSocket::SOCKET_TIMEOUT_USECONDS = 0; // 0us
    const uint32_t UdpPktSocket::UDP_DEFAULT_RCVBUFSIZE = 212992; // 208KB used in linux by default
    //const uint32_t UdpPktSocket::UDP_LARGE_RCVBUFSIZE = 8388608; // 8MB used for large data

    const std::string UdpPktSocket::kClassName("UdpPktSocket");

	UdpPktSocket::UdpPktSocket(const bool& need_timeout) : need_timeout_(need_timeout)
	{
		// Create UDP socket
		createUdpsock_();
	}

    UdpPktSocket::UdpPktSocket(const bool& need_timeout, const NetworkAddr& host_addr) : need_timeout_(need_timeout)
    {
        assert(host_addr.isValid() == true);
        
		// Not support to bind a specific host IP for UDP server
        std::string host_ipstr = host_addr.getIpstr();
		if (host_ipstr != Util::ANY_IPSTR)
		{
			std::ostringstream oss;
			oss << "NOT support to bind a specific host IP address " << host_ipstr << " for UDP server now!";	
			Util::dumpErrorMsg(kClassName, oss.str());
			exit(1);
		}

		// Create UDP socket
		createUdpsock_();

		// Prepare for listening on the host address
		enableReuseaddr_();
		bindSockaddr_(host_addr);
    }

	UdpPktSocket::~UdpPktSocket() {}

	void UdpPktSocket::sendto(const DynamicArray& pkt_payload, const NetworkAddr& remote_addr)
	{
        assert(remote_addr.isValid() == true);

		// Not support broadcast for UDP client
        std::string remote_ipstr = remote_addr.getIpstr();
		if (remote_ipstr == Util::ANY_IPSTR)
		{
			std::ostringstream oss;
			oss << "invalid remote ipstr of " << remote_ipstr << " for sending a single UDP packet!";	
			Util::dumpErrorMsg(kClassName, oss.str());
			exit(1);
		}

		// Prepare sockaddr based on remote address
		struct sockaddr_in remote_sockaddr;
        memset((void *)&remote_sockaddr, 0, sizeof(remote_sockaddr));
        remote_sockaddr.sin_family = AF_INET;
		inet_pton(AF_INET, remote_ipstr.c_str(), &(remote_sockaddr.sin_addr));
        uint16_t remote_port = remote_addr.getPort();
        remote_sockaddr.sin_port = htons(remote_port);

		// Send UDP packet
		int flags = 0;
		int return_code = sendto(sockfd_, pkt_payload.getBytes().data(), pkt_payload.getSize(), flags, (struct sockaddr*)(&remote_sockaddr), sizeof(remote_sockaddr));
		if (return_code < 0) {
			std::ostringstream oss;
            oss << "failed to send " << pkt_payload.getSize() << " bytes to remote address with ip " << remote_ipstr << " and port " << remote_port << " (errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
		}

		return;
	}

	bool UdpPktSocket::recvfrom(DynamicArray& pkt_payload, NetworkAddr& remote_addr)
	{
        bool is_timeout = false;

		// Prepare sockaddr for remote address
		struct sockaddr_in remote_sockaddr;
		memset((void *)&remote_sockaddr, 0, sizeof(remote_sockaddr));

        // Prepare for packet payload
        char tmp_pkt_payload[Util::UDP_MAX_PKT_PAYLOAD];
        memset(tmp_pkt_payload, 0, Util::UDP_MAX_PKT_PAYLOAD);

		// Try to receive the UDP packet
		int flags = 0;
		int recvsize = recvfrom(sockfd_, tmp_pkt_payload, Util::UDP_MAX_PKT_PAYLOAD, flags, (struct sockaddr *)&remote_sockaddr, sizeof(remote_sockaddr));
		if (recvsize < 0) { // Failed to receive a UDP packet
			if (need_timeout_ && (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN)) {
				is_timeout = true;
			}
			else {
				std::ostringstream oss;
				oss << "failed to receive a UDP packet (errno: " << errno << ")!";
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}
		}
		else // Successfully receive a UDP packet
		{
            // Copy into pkt_payload
            pkt_payload.write(0, tmp_pkt_payload, recvsize);

			// Set remote address for successful packet receiving
			char remote_ipcstr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(remote_sockaddr.sin_addr), remote_ipcstr, INET_ADDRSTRLEN);
            remote_addr.setIpstr(std::string(remote_ipcstr));
            remote_addr.setPort(ntohs(remote_sockaddr.sin_port));
            remote_addr.setValid();
		}

		return is_timeout;
	}

    void UdpPktSocket::createUdpsock() {
		// Create UDP socket
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0)
        {
            std::ostringstream oss;
            oss << "failed to create a UDP socket (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Disable UDP checksum
        //int disable = 1;
        //if (setsockopt(sockfd, SOL_SOCKET, SO_NO_CHECK, (void*)&disable, sizeof(disable)) < 0)
        //{
        //    std::ostringstream oss;
        //    oss << "failed to disable UDP checksum (errno: " << errno << ")!";
        //    Util::dumpErrorMsg(kClassName, oss.str());
        //    exit(1);
        //}

        // Set timeout for recvfrom/accept of UDP client/server
        if (need_timeout_)
        {
            setTimeout();
        }

        // Set UDP receive buffer size
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &UDP_DEFAULT_RCVBUFSIZE, sizeof(int)) < 0)
        {
            std::ostringstream oss;
            oss << "failed to set UDP receive bufsize (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }

    void UdpPktSocket::setTimeout() {
		// Set timeout for recvfrom/accept of UDP client/server
        struct timeval tv;
        tv.tv_sec = SOCKET_TIMEOUT_SECONDS;
        tv.tv_usec =  SOCKET_TIMEOUT_USECONDS;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            std::ostringstream oss;
            oss << "failed to set timeout (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void UdpPktSocket::enableReuseaddr()
    {
        // Reuse the occupied port for the last created socket instead of being crashed
        const int trueFlag = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) < 0) {
            std::ostringstream oss;
            oss << "failed to enable reuseaddr" << " (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void UdpPktSocket::bindSockaddr(const NetworkAddr& host_addr) {
        std::string host_ipstr = host_addr.getIpstr();
        uint16_t host_port = host_addr.getPort();

		// Prepare sockaddr based on host address
        struct sockaddr_in host_sockaddr;
        memset((void *)&host_sockaddr, 0, sizeof(host_sockaddr));
        host_sockaddr.sin_family = AF_INET;
        //host_sockaddr.sin_addr.s_addr = INADDR_ANY; // set local IP as any IP address
		inet_pton(AF_INET, host_ipstr.c_str(), &(host_sockaddr.sin_addr));
        host_sockaddr.sin_port = htons(host_port);

		// Bind host address to wait for packets from UDP clients
        if ((bind(sockfd_, (struct sockaddr*)&host_sockaddr, sizeof(host_sockaddr))) < 0) {
            std::ostringstream oss;
            oss << "failed to bind UDP socket on ip " << host_ip << " and port " << host_port << " (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
}