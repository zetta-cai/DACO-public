#include "network/socket_wrapper.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const uint32_t UdpSocketWrapper::SOCKET_TIMEOUT_SECONDS = 5; // 5s
    const uint32_t UdpSocketWrapper::SOCKET_TIMEOUT_USECONDS = 0; // 0us
    const uint32_t UdpSocketWrapper::UDP_PAYLOAD_MAXSIZE = 65507; // 65535(ipmax) - 20(iphdr) - 8(udphdr)
    const uint32_t UdpSocketWrapper::UDP_DEFAULT_RCVBUFSIZE = 212992; // 208KB used in linux by default
    //const uint32_t UdpSocketWrapper::UDP_LARGE_RCVBUFSIZE = 8388608; // 8MB used for large data

    const std::string UdpSocketWrapper::kClassName("UdpSocketWrapper");

    UdpSocketWrapper(const SocketRole& role, const bool& need_timeout, const std::string& ipstr, const uint16_t& udp_port) : role_(role), need_timeout_(need_timeout), udp_host_ipstr_(role == SocketRole::kSocketServer ? ipstr : ""), udp_host_port_(role == SocketRole::kSocketServer ? port : 0)
    {
        if (role_ == SocketRole::kSocketClient)
        {
			// Not support broadcast for UDP client 
			if (udp_remote_ipstr_ == Util::ANY_IPSTR)
			{
				std::ostringstream oss;
				oss << "invalid remote ipstr of " << udp_remote_ipstr_ << " for UDP client!";	
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// Remote address is fixed to UDP client
			udp_remote_ipstr_ = ipstr;
			udp_remote_port_ = port;
			is_receive_remote_address = true; // always true for UDP client

			// Create UDP socket
			createUdpsock();
        }
        else if (role_ == SocketRole::kSocketServer)
        {
			// Not support to bind a specific host IP for UDP server
			if (udp_host_ipstr_ != Util::ANY_IPSTR)
			{
				std::ostringstream oss;
				oss << "NOT support to bind a specific host IP address " << udp_host_ipstr_ << " for UDP server now!";	
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// UDP port must be > 4096
			if (udp_host_port_ <= 4096)
			{
				std::ostringstream oss;
				oss << "invalid host port of " << udp_host_port_ << " which should be > 4096!";	
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// Remote address is dynamically changed by recvfrom for UDP server
			udp_remote_ipstr_ = "";
			udp_remote_port_ = 0;
			is_receive_remote_address = false; // changed for UDP server

			// Create UDP socket
			createUdpsock();

			// Prepare for listening on the host address
            enableReuseaddr();
            bindSockaddr();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid SocketRole: " << static_cast<int>(role_);
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

	void UdpSocketWrapper::sendto(const std::vector<char>& buf)
	{
		// Must with valid remote address
		if (is_receive_remote_address == false)
		{
			std::ostringstream oss;
            oss << "NO remote address for SocketRole: " << static_cast<int>(role_);
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
		}

		// Prepare sockaddr based on remote address
		struct sockaddr_in remote_sockaddr;
        memset((void *)&remote_sockaddr, 0, sizeof(remote_sockaddr));
        remote_sockaddr.sin_family = AF_INET;
		inet_pton(AF_INET, udp_remote_ipstr_.c_str(), &(remote_sockaddr.sin_addr));
        remote_sockaddr.sin_port = htons(udp_remote_port_);

		// Send UDP packet
		int flags = 0;
		int return_code = sendto(sockfd_, payload.data(), payload.size(), flags, (struct sockaddr*)(&remote_sockaddr), sizeof(remote_sockaddr));
		if (return_code < 0) {
			std::ostringstream oss;
            oss << "failed to send " << payload.size() << " bytes to remote address with ip " << udp_remote_ipstr_ << " and port " << udp_remote_port_ << " (error code: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
		}

		// Reset is_receive_remote_address for UDP server
		if (role_ == SocketRole::kSocketServer)
		{
			is_receive_remote_address = false; // to be set by the next recvfrom
		}
		return;
	}

    void UdpSocketWrapper::createUdpsock() {
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

    void UdpSocketWrapper::setTimeout() {
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

    void UdpSocketWrapper::enableReuseaddr()
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

    void UdpSocketWrapper::bindSockaddr() {
		// Prepare sockaddr based on host address
        struct sockaddr_in host_sockaddr;
        memset((void *)&host_sockaddr, 0, sizeof(host_sockaddr));
        host_sockaddr.sin_family = AF_INET;
        //host_sockaddr.sin_addr.s_addr = INADDR_ANY; // set local IP as any IP address
		inet_pton(AF_INET, udp_host_ipstr_.c_str(), &(host_sockaddr.sin_addr));
        host_sockaddr.sin_port = htons(udp_host_port_);

		// Bind host address to wait for packets from UDP clients
        if ((bind(sockfd_, (struct sockaddr*)&host_sockaddr, sizeof(host_sockaddr))) < 0) {
            std::ostringstream oss;
            oss << "failed to bind UDP socket on port " << udp_port_ << " (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
}






/*#include <arpa/inet.h> // inetaddr conversion; endianess conversion
#include <netinet/tcp.h> // TCP_NODELAY

// udp

bool udprecvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, int &recvsize, const char* role) {
	bool need_timeout = false;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec =  0;
	socklen_t tvsz = sizeof(tv);
	int res = getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &tvsz);
	UNUSED(res);
	if (tv.tv_sec != 0 || tv.tv_usec != 0) {
		need_timeout = true;
	}

	bool is_timeout = false;
	recvsize = recvfrom(sockfd, buf, len, flags, (struct sockaddr *)src_addr, addrlen);
	if (recvsize < 0) {
		if (need_timeout && (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN)) {
			recvsize = 0;
			is_timeout = true;
		}
		else {
			printf("[%s] error of recvfrom, errno: %d!\n", role, errno);
			exit(-1);
		}
	}
	return is_timeout;
}

void udpsendlarge_udpfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role) {
	udpsendlarge(sockfd, buf, len, flags, dest_addr, addrlen, role, 0, UDP_FRAGMENT_MAXSIZE);
}

void udpsendlarge_ipfrag(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize) {
	udpsendlarge(sockfd, buf, len, flags, dest_addr, addrlen, role, frag_hdrsize, IP_FRAGMENT_MAXSIZE);
}

void udpsendlarge(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role, size_t frag_hdrsize, size_t frag_maxsize) {
	// (1) buf[0:frag_hdrsize] + <cur_fragidx, max_fragnum> as final header of each fragment payload
	// (2) frag_maxsize is the max size of fragment payload (final fragment header + fragment body), yet not including ethernet/ipv4/udp header
	INVARIANT(len >= frag_hdrsize);
	size_t final_frag_hdrsize = frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
	size_t frag_bodysize = frag_maxsize - final_frag_hdrsize;
	size_t total_bodysize = len - frag_hdrsize;
	size_t fragnum = 1;
	if (likely(total_bodysize > 0)) {
		fragnum = (total_bodysize + frag_bodysize - 1) / frag_bodysize;
	}
	INVARIANT(fragnum > 0);
	//printf("frag_hdrsize: %d, final_frag_hdrsize: %d, frag_maxsize: %d, frag_bodysize: %d, total_bodysize: %d, fragnum: %d\n", frag_hdrsize, final_frag_hdrsize, frag_maxsize, frag_bodysize, total_bodysize, fragnum);

	// <frag_hdrsize, cur_fragidx, max_fragnum, frag_bodysize>
	char fragbuf[frag_maxsize];
	memset(fragbuf, 0, frag_maxsize);
	memcpy(fragbuf, buf, frag_hdrsize); // fixed fragment header
	uint16_t cur_fragidx = 0;
	uint16_t max_fragnum = uint16_t(fragnum);
	size_t buf_sentsize = frag_hdrsize;
	for (; cur_fragidx < max_fragnum; cur_fragidx++) {
		memset(fragbuf + frag_hdrsize, 0, frag_maxsize - frag_hdrsize);

		// prepare for final fragment header
		//// NOTE: UDP fragmentation is processed by end-hosts instead of switch -> no need for endianess conversion
		//memcpy(fragbuf + frag_hdrsize, &cur_fragidx, sizeof(uint16_t));
		//memcpy(fragbuf + frag_hdrsize + sizeof(uint16_t), &max_fragnum, sizeof(uint16_t));
		// NOTE: to support large value, switch needs to parse fragidx and fragnum -> NEED endianess conversion now
		uint16_t bigendian_cur_fragidx = htons(cur_fragidx); // littleendian -> bigendian for large value
		memcpy(fragbuf + frag_hdrsize, &bigendian_cur_fragidx, sizeof(uint16_t));
		uint16_t bigendian_max_fragnum = htons(max_fragnum); // littleendian -> bigendian for large value
		memcpy(fragbuf + frag_hdrsize + sizeof(uint16_t), &bigendian_max_fragnum, sizeof(uint16_t));

		// prepare for fragment body
		int cur_frag_bodysize = frag_bodysize;
		if (cur_fragidx == max_fragnum - 1) {
			cur_frag_bodysize = total_bodysize - frag_bodysize * cur_fragidx;
		}
		memcpy(fragbuf + final_frag_hdrsize, buf + buf_sentsize, cur_frag_bodysize);
		buf_sentsize += cur_frag_bodysize;

		//printf("cur_fragidx: %d, max_fragnum: %d, cur_fragsize: %d\n", cur_fragidx, max_fragnum, final_frag_hdrsize + cur_frag_bodysize);
		udpsendto(sockfd, fragbuf, final_frag_hdrsize + cur_frag_bodysize, flags, dest_addr, addrlen, role);
	}
}

bool udprecvlarge_udpfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role) {
	return udprecvlarge(methodid, sockfd, buf, flags, src_addr, addrlen, role, UDP_FRAGMENT_MAXSIZE, UDP_FRAGTYPE, NULL);
}

bool udprecvlarge_ipfrag(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, pkt_ring_buffer_t *pkt_ring_buffer_ptr) {
	return udprecvlarge(methodid, sockfd, buf, flags, src_addr, addrlen, role, IP_FRAGMENT_MAXSIZE, IP_FRAGTYPE, pkt_ring_buffer_ptr);
}

// NOTE: receive large packet from one source
bool udprecvlarge(method_t methodid, int sockfd, dynamic_array_t &buf, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen, const char* role, size_t frag_maxsize, int fragtype, pkt_ring_buffer_t *pkt_ring_buffer_ptr) {

	bool is_timeout = false;
	struct sockaddr_in tmp_src_addr;
	socklen_t tmp_addrlen = sizeof(struct sockaddr_in);

	size_t frag_hdrsize = 0;
	size_t final_frag_hdrsize = 0;
	size_t frag_totalsize = frag_maxsize; // frag_maxsize (final_frag_hdrsize of sent optype + frag_bodysize) + extra packet headers if any = final_frag_hdrsize of received optype + frag_bodysize
	size_t frag_bodysize = 0;

	// frag_maxsize = frag_hdrsize of sent optype + 16b fragidx + 16b fragnum + payload of frag_bodysize (used by udpsendlarge)
	// NOTE: programmable switch may introduce extra packet headers
	size_t fragbuf_maxsize = frag_maxsize;
	if (fragtype == IP_FRAGTYPE) {
		fragbuf_maxsize += IP_FRAGMENT_RESERVESIZE; // assign more bytes for extra packet headers
	}

	char fragbuf[fragbuf_maxsize];
	int frag_recvsize = 0;
	bool is_first = true;
	uint16_t max_fragnum = 0;
	uint16_t cur_fragnum = 0;

	// ONLY for udprecvlarge_ipfrag
	bool is_used_by_server = (pkt_ring_buffer_ptr != NULL);
	netreach_key_t largepkt_key;
	packet_type_t largepkt_optype;
	uint16_t largepkt_clientlogicalidx; // ONLY for server
	uint32_t largepkt_fragseq; // ONLY for server
	//uint32_t server_unmatchedcnt = 0; // ONLY for server
	struct sockaddr_in largepkt_clientaddr;
	socklen_t largepkt_clientaddrlen;

	// pop pkt from pkt_ring_buffer if any ONLY for IP_FRAGTYPE by server
	if (fragtype == IP_FRAGTYPE && is_used_by_server) {
		bool has_pkt = pkt_ring_buffer_ptr->pop(largepkt_optype, largepkt_key, buf, cur_fragnum, max_fragnum, tmp_src_addr, tmp_addrlen, largepkt_clientlogicalidx, largepkt_fragseq);
		if (has_pkt) { // if w/ pkt in pkt_ring_buffer
			// Copy src address of the first packet for both large and not-large packet
			if (src_addr != NULL) {
				*src_addr = tmp_src_addr;
			}
			if (addrlen != NULL) {
				*addrlen = tmp_addrlen;
			}

			if (max_fragnum == 0) { // small packet
				return false;
			}
			else { // large packet
				if (cur_fragnum >= max_fragnum) { // w/ all fragments
					//printf("pop a complete large packet %x of key %x from client %d\n", int(largepkt_optype), largepkt_key.keyhihi, largepkt_clientlogicalidx);
					//if (largepkt_clientlogicalidx == 16) {
					//	CUR_TIME(process_t2);
					//	DELTA_TIME(process_t2, process_t1, process_t3);
					//	double process_time = GET_MICROSECOND(process_t3);
					//	printf("process time for client 16: %f\n", process_time);
					//	fflush(stdout);
					//}
					return false;
				}
				else { // need to receive remaining fragments
					is_first = false; // we do NOT need to process the first packet
					INVARIANT(is_packet_with_largevalue(largepkt_optype) == true);
					frag_totalsize = get_frag_totalsize(methodid, largepkt_optype, frag_maxsize); // update frag_totalsize for extra packet headers if any
					frag_hdrsize = get_frag_hdrsize(methodid, largepkt_optype);
					final_frag_hdrsize = frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
					frag_bodysize = frag_totalsize - final_frag_hdrsize;
					largepkt_clientaddr = tmp_src_addr;
					largepkt_clientaddrlen = tmp_addrlen;
					//printf("pop incomplete large packet %x of key %x from client %d w/ cur_fragnum %d max_fragnum %d\n", int(largepkt_optype), largepkt_key.keyhihi, largepkt_clientlogicalidx, cur_fragnum, max_fragnum);
				}
			}
		}
	}

	while (true) {
		if (is_first) {
			is_timeout = udprecvfrom(sockfd, fragbuf, fragbuf_maxsize, flags, &tmp_src_addr, &tmp_addrlen, frag_recvsize, role);
			if (is_timeout) {
				// NOTE: we do NOT need to push packet back into PktRingBuffer as even the first packet is NOT received
				break;
			}

			// Copy src address of the first packet for both large and not-large packet
			if (src_addr != NULL) {
				*src_addr = tmp_src_addr;
			}
			if (addrlen != NULL) {
				*addrlen = tmp_addrlen;
			}

			// Judge whether the current packet is large or not
			bool is_largepkt = false;
			if (fragtype == IP_FRAGTYPE) { // NOTE: server and client (ONLY for GETREQ) do NOT know whether the packet will be large or not before udprecvlarge_ipfrag
				packet_type_t tmp_optype = get_packet_type(fragbuf, frag_recvsize);
				is_largepkt = is_packet_with_largevalue(tmp_optype);
				if (is_largepkt) {
					frag_hdrsize = get_frag_hdrsize(methodid, tmp_optype);
				}
			}
			else if (fragtype == UDP_FRAGTYPE) {
				frag_hdrsize = 0;
				is_largepkt = true; // SNAPSHOT_GETDATA_ACK or SNAPSHOT_SENDDATA MUST be large packet
			}
			else {
				printf("[ERROR] invalid fragtype in udprecvlarge: %d\n", fragtype);
				exit(-1);
			}

			if (!is_largepkt) {
				// NOT large packet -> jump out of the while loop -> behave similar as udprecvfrom to receive a single small packet
				buf.dynamic_memcpy(0, fragbuf, frag_recvsize);
				INVARIANT(!is_timeout);
				break; // <=> return;
			}

			// NOTE: now the current packet MUST be large

			// Save optype and key [and client_logical_idx] ONLY for udprecvlarge_ipfrag to filter unnecessary packet fro client [and server]
			if (fragtype == IP_FRAGTYPE) {
				largepkt_key = get_packet_key(methodid, fragbuf, frag_recvsize);
				largepkt_optype = get_packet_type(fragbuf, frag_recvsize);
				if (is_used_by_server) {
					frag_totalsize = get_frag_totalsize(methodid, largepkt_optype, frag_maxsize); // update frag_totalsize for extra packet headers if any
					largepkt_clientlogicalidx = get_packet_clientlogicalidx(methodid, fragbuf, frag_recvsize);
					largepkt_fragseq = get_packet_fragseq(methodid, fragbuf, frag_recvsize);
					largepkt_clientaddr = tmp_src_addr;
					largepkt_clientaddrlen = tmp_addrlen;
					//if (largepkt_clientlogicalidx == 16) {
					//	CUR_TIME(process_t1);
					//}
				}
			}

			// Calculate fraghdrsize, final fraghdrsize, and fragbodysize based on optype
			final_frag_hdrsize = frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
			frag_bodysize = frag_totalsize - final_frag_hdrsize;
			INVARIANT(final_frag_hdrsize != 0 && frag_bodysize != 0);
			INVARIANT(size_t(frag_recvsize) >= final_frag_hdrsize && size_t(frag_recvsize) <= frag_totalsize);
			//printf("frag_hdrsize: %d, final_frag_hdrsize: %d, frag_maxsize: %d, frag_bodysize: %d\n", frag_hdrsize, final_frag_hdrsize, frag_maxsize, frag_bodysize);

			// Copy fraghdr and get max fragnum
			buf.dynamic_memcpy(0, fragbuf, frag_hdrsize);
			memcpy(&max_fragnum, fragbuf + frag_hdrsize + sizeof(uint16_t), sizeof(uint16_t));
			max_fragnum = ntohs(max_fragnum); // bigendian -> littleendian for large value
			is_first = false; // NOTE: we MUST set is_first = false after timeout checking
		}
		else { // NOTE: access the following code block ONLY if we are pursuiting a large packet
			//if (server_unmatchedcnt >= SERVER_UNMATCHEDCNT_THRESHOLD) {
			//	printf("server.worker timeouts w/ server_unmatchedcnt %d vs. SERVER_UNMATCHEDCNT_THRESHOLD %d\n", server_unmatchedcnt, SERVER_UNMATCHEDCNT_THRESHOLD);
			//	is_timeout = true;
			//	break;
			//}

			is_timeout = udprecvfrom(sockfd, fragbuf, fragbuf_maxsize, flags, &tmp_src_addr, &tmp_addrlen, frag_recvsize, role);
			if (is_timeout) {
				break;
			}
	
			// Filter unexpected packets ONLY for udprecvlarge_ipfrag
			if (fragtype == IP_FRAGTYPE) {
				netreach_key_t tmp_nonfirstpkt_key = get_packet_key(methodid, fragbuf, frag_recvsize);
				packet_type_t tmp_nonfirstpkt_optype = get_packet_type(fragbuf, frag_recvsize);
				if (!is_used_by_server) { // used by client
					if (tmp_nonfirstpkt_key != largepkt_key || tmp_nonfirstpkt_optype != largepkt_optype) { // unmatched packet
						continue; // skip current unmatched packet, go to receive next one
					}
					// else {} // matched packet
				}
				else { // used by server
					bool tmp_stat = false;
					if (!is_packet_with_largevalue(tmp_nonfirstpkt_optype)) { // not-large packet
						// Push small request into PktRingBuffer
						//printf("push a small request %x into pkt ring buffer\n", int(tmp_nonfirstpkt_optype));
						tmp_stat = pkt_ring_buffer_ptr->push(tmp_nonfirstpkt_optype, tmp_nonfirstpkt_key, fragbuf, frag_recvsize, tmp_src_addr, tmp_addrlen);
						if (!tmp_stat) {
							printf("[ERROR] overflow of pkt_ring_buffer when push optype %x\n", optype_t(tmp_nonfirstpkt_optype));
							exit(-1);
						}
						//server_unmatchedcnt += 1;
						continue;
					}
					else { // large packet
						if (is_packet_with_clientlogicalidx(tmp_nonfirstpkt_optype)) { // large packet for server
							uint16_t tmp_nonfirstpkt_clientlogicalidx = get_packet_clientlogicalidx(methodid, fragbuf, frag_recvsize);
							uint32_t tmp_nonfirstpkt_fragseq = get_packet_fragseq(methodid, fragbuf, frag_recvsize);
							if (tmp_nonfirstpkt_clientlogicalidx != largepkt_clientlogicalidx) { // from different client
								// calculate fraghdrsize and fragbodysize for the large packet from different client
								uint32_t tmp_nonfirstpkt_frag_totalsize = get_frag_totalsize(methodid, tmp_nonfirstpkt_optype, frag_maxsize);
								uint32_t tmp_nonfirstpkt_frag_hdrsize = get_frag_hdrsize(methodid, tmp_nonfirstpkt_optype);
								uint32_t tmp_nonfirstpkt_final_frag_hdrsize = tmp_nonfirstpkt_frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
								uint32_t tmp_nonfirstpkt_frag_bodysize = tmp_nonfirstpkt_frag_totalsize - tmp_nonfirstpkt_final_frag_hdrsize;
								INVARIANT(tmp_nonfirstpkt_final_frag_hdrsize != 0 && tmp_nonfirstpkt_frag_bodysize != 0);
								INVARIANT(size_t(frag_recvsize) >= tmp_nonfirstpkt_final_frag_hdrsize && size_t(frag_recvsize) <= tmp_nonfirstpkt_frag_totalsize);

								// calculate maxfragnum and curfragidx for the large packet from different client
								uint16_t tmp_nonfirstpkt_max_fragnum = 0, tmp_nonfirstpkt_cur_fragidx = 0;
								memcpy(&tmp_nonfirstpkt_max_fragnum, fragbuf + tmp_nonfirstpkt_frag_hdrsize + sizeof(uint16_t), sizeof(uint16_t));
								tmp_nonfirstpkt_max_fragnum = ntohs(tmp_nonfirstpkt_max_fragnum); // bigendian -> littleendian for large value
								memcpy(&tmp_nonfirstpkt_cur_fragidx, fragbuf + tmp_nonfirstpkt_frag_hdrsize, sizeof(uint16_t));
								tmp_nonfirstpkt_cur_fragidx = ntohs(tmp_nonfirstpkt_cur_fragidx); // bigendian -> littleendian for large value

								// judge whether it is a new large packet or existing large packet
								bool is_clientlogicalidx_exist = pkt_ring_buffer_ptr->is_clientlogicalidx_exist(tmp_nonfirstpkt_clientlogicalidx);
								if (is_clientlogicalidx_exist) { // update large packet received before
									// Update existing large packet in PktRingBuffer
									INVARIANT(fragtype == IP_FRAGTYPE);
									//printf("update a large packet %x of key %x from client %d into pkt ring buffer\n", int(tmp_nonfirstpkt_optype), tmp_nonfirstpkt_key.keyhihi, tmp_nonfirstpkt_clientlogicalidx);
									if (tmp_nonfirstpkt_cur_fragidx != 0) {
										pkt_ring_buffer_ptr->update_large(tmp_nonfirstpkt_optype, tmp_nonfirstpkt_key, fragbuf, tmp_nonfirstpkt_frag_hdrsize, tmp_nonfirstpkt_frag_hdrsize + tmp_nonfirstpkt_cur_fragidx * tmp_nonfirstpkt_frag_bodysize, fragbuf + tmp_nonfirstpkt_final_frag_hdrsize, frag_recvsize - tmp_nonfirstpkt_final_frag_hdrsize, tmp_src_addr, tmp_addrlen, tmp_nonfirstpkt_clientlogicalidx, tmp_nonfirstpkt_fragseq, false);
									}
									else { // memcpy fraghdr again for fragment 0 to ensure correct seq for farreach/distfarreach/netcache/distcache
										pkt_ring_buffer_ptr->update_large(tmp_nonfirstpkt_optype, tmp_nonfirstpkt_key, fragbuf, tmp_nonfirstpkt_frag_hdrsize, tmp_nonfirstpkt_frag_hdrsize + tmp_nonfirstpkt_cur_fragidx * tmp_nonfirstpkt_frag_bodysize, fragbuf + tmp_nonfirstpkt_final_frag_hdrsize, frag_recvsize - tmp_nonfirstpkt_final_frag_hdrsize, tmp_src_addr, tmp_addrlen, tmp_nonfirstpkt_clientlogicalidx, tmp_nonfirstpkt_fragseq, true);
									}
								}
								else { // add new large packet
									//if (tmp_nonfirstpkt_clientlogicalidx == 16) {
									//	CUR_TIME(process_t1);
									//}
									// Push large packet into PktRingBuffer
									//printf("push a large packet %x of key %x from client %d into pkt ring buffer\n", int(tmp_nonfirstpkt_optype), tmp_nonfirstpkt_key.keyhihi, tmp_nonfirstpkt_clientlogicalidx);
									tmp_stat = pkt_ring_buffer_ptr->push_large(tmp_nonfirstpkt_optype, tmp_nonfirstpkt_key, fragbuf, tmp_nonfirstpkt_frag_hdrsize, tmp_nonfirstpkt_frag_hdrsize + tmp_nonfirstpkt_cur_fragidx * tmp_nonfirstpkt_frag_bodysize, fragbuf + tmp_nonfirstpkt_final_frag_hdrsize, frag_recvsize - tmp_nonfirstpkt_final_frag_hdrsize, 1, tmp_nonfirstpkt_max_fragnum, tmp_src_addr, tmp_addrlen, tmp_nonfirstpkt_clientlogicalidx, tmp_nonfirstpkt_fragseq);
									if (!tmp_stat) {
										printf("[ERROR] overflow of pkt_ring_buffer when push_large optype %x clientlogicalidx %d\n", optype_t(tmp_nonfirstpkt_optype), tmp_nonfirstpkt_clientlogicalidx);
										exit(-1);
									}
								}
								//server_unmatchedcnt += 1;
								continue;
							}
							else { // from the same client
								if (tmp_nonfirstpkt_key != largepkt_key || !is_same_optype(tmp_nonfirstpkt_optype, largepkt_optype) || tmp_nonfirstpkt_fragseq < largepkt_fragseq) { // unmatched packet
									printf("[WARNING] unmatched large packet %x (expect: %x) from client %d of key %x (expect: %x) with fragseq %d (expect %d)\n", \
											int(tmp_nonfirstpkt_optype), int(largepkt_optype), tmp_nonfirstpkt_clientlogicalidx, tmp_nonfirstpkt_key.keyhihi, largepkt_key.keyhihi, tmp_nonfirstpkt_fragseq, largepkt_fragseq);
									continue; // skip current unmatched packet, go to receive next one
								}
								else { // matched packet w/ fragseq >= largepkt_fragseq
									if (tmp_nonfirstpkt_fragseq > largepkt_fragseq) {
										printf("[WARNING] socket helper receives larger fragseq %d > %d of key %x from client %d due to client-side timeout-and-retry\n", tmp_nonfirstpkt_fragseq, largepkt_fragseq, largepkt_key.keyhihi, largepkt_clientlogicalidx);
										largepkt_fragseq = tmp_nonfirstpkt_fragseq;
										cur_fragnum = 0; // increased by 1 later; treat the current fragment w/ larger fragseq as the first fragment
									}
									//else {} // fragseq == largepkt_fragseq
									//server_unmatchedcnt = 0;
								}
							}
						}
						else { // packet NOT for server
							printf("[ERROR] invalid packet type received by server: %x\n", optype_t(tmp_nonfirstpkt_optype));
							exit(-1);
						} // large packet to server w/ clientlogicalidx
					} // large/small to server
				} // to client/server
			} // IP_FRAGTYPE

			// NOTE: ONLY matched packet to client OR matched large packet from the same client to server can arrive here
			INVARIANT(size_t(frag_recvsize) >= final_frag_hdrsize);
		}

		// NOTE: ONLY large packet can access the following code

		uint16_t cur_fragidx = 0;
		memcpy(&cur_fragidx, fragbuf + frag_hdrsize, sizeof(uint16_t));
		cur_fragidx = ntohs(cur_fragidx); // bigendian -> littleendian for large value
		INVARIANT(cur_fragidx < max_fragnum);
		//printf("cur_fragidx: %d, max_fragnum: %d, frag_recvsize: %d, buf_offset: %d, copy_size: %d\n", cur_fragidx, max_fragnum, frag_recvsize, cur_fragidx * frag_bodysize, frag_recvsize - final_frag_hdrsize);
		
		//printf("cur_fragidx %d cur_fragnum %d max_fragnum %d key %x client %d\n", cur_fragidx, cur_fragnum, max_fragnum, largepkt_key.keyhihi, largepkt_clientlogicalidx);
		//fflush(stdout);

		buf.dynamic_memcpy(0 + frag_hdrsize + cur_fragidx * frag_bodysize, fragbuf + final_frag_hdrsize, frag_recvsize - final_frag_hdrsize);

		// memcpy fraghdr again for fragment 0 to ensure correct seq for farreach/distfarreach/netcache/distcache
		if (fragtype == IP_FRAGTYPE && cur_fragidx == 0) {
			buf.dynamic_memcpy(0, fragbuf, frag_hdrsize);
		}

		cur_fragnum += 1;
		if (cur_fragnum >= max_fragnum) {
			//if (largepkt_clientlogicalidx == 16) {
			//	CUR_TIME(process_t2);
			//	DELTA_TIME(process_t2, process_t1, process_t3);
			//	double process_time = GET_MICROSECOND(process_t3);
			//	printf("process time for client 16: %f\n", process_time);
			//	fflush(stdout);
			//}
			break;
		}
	}

	if (is_timeout) {
		if (fragtype == IP_FRAGTYPE && is_used_by_server) {
			if (!is_first) { // we are waiting for remaining fragments of a large packet
				INVARIANT(pkt_ring_buffer_ptr->is_clientlogicalidx_exist(largepkt_clientlogicalidx) == false);
				INVARIANT(cur_fragnum >= 1);
				// Push back current large packet into PktRingBuffer
				//printf("[WARNING] push back current large packet %x of key %x from client %d into pkt ring buffer\n", int(largepkt_optype), largepkt_key.keyhihi, largepkt_clientlogicalidx);
				bool tmp_stat = pkt_ring_buffer_ptr->push_large(largepkt_optype, largepkt_key, buf.array(), frag_hdrsize, frag_hdrsize, buf.array() + frag_hdrsize, buf.size() - frag_hdrsize, cur_fragnum, max_fragnum, largepkt_clientaddr, largepkt_clientaddrlen, largepkt_clientlogicalidx, largepkt_fragseq);
				if (!tmp_stat) {
					printf("[ERROR] overflow of pkt_ring_buffer when push_large optype %x clientlogicalidx %d\n", optype_t(largepkt_optype), largepkt_clientlogicalidx);
					exit(-1);
				}
			}
		}
		buf.clear();
	}

	return is_timeout;
}*/