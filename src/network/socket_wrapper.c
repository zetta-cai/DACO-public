#include "network/socket_wrapper.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const uint32_t SocketWrapper::SOCKET_TIMEOUT_SECONDS = 5; // 5s
    const uint32_t SocketWrapper::SOCKET_TIMEOUT_USECONDS = 0; // 0us
    const uint32_t SocketWrapper::UDP_PAYLOAD_MAXSIZE = 65507; // 65535(ipmax) - 20(iphdr) - 8(udphdr)
    const uint32_t SocketWrapper::UDP_DEFAULT_RCVBUFSIZE = 212992; // 208KB used in linux by default
    //const uint32_t SocketWrapper::UDP_LARGE_RCVBUFSIZE = 8388608; // 8MB used for large data

    const std::string SocketWrapper::kClassName("SocketWrapper");

    SocketWrapper(const SocketRole& role, const bool& need_timeout, const uint16_t& port) : role_(role), need_timeout_(need_timeout), port_(port)
    {
        if (role_ == SocketRole::kSocketClient)
        {
            createUdpsock();
        }
        else if (role_ == SocketRole::kSocketServer)
        {
            createUdpsock();
            if (port_ == 0)
            {
                Util::dumpErrorMsg(kClassName, "NOT set port for kSocketServer!");
                exit(1);
            }
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

    void SocketWrapper::createUdpsock() {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0)
        {
            std::ostringstream oss;
            oss << "failed to create a UDP socket (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Disable UDP/TCP checksum
        //int disable = 1;
        //if (setsockopt(sockfd, SOL_SOCKET, SO_NO_CHECK, (void*)&disable, sizeof(disable)) < 0)
        //{
        //    std::ostringstream oss;
        //    oss << "failed to disable UDP checksum (errno: " << errno << ")!";
        //    Util::dumpErrorMsg(kClassName, oss.str());
        //    exit(1);
        //}

        // Set timeout for recvfrom/accept of UDP/TCP
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

    void SocketWrapper::setTimeout() {
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

    void SocketWrapper::enableReuseaddr()
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

    void SocketWrapper::bindSockaddr() {
        struct sockaddr_in sockaddr;
        memset((void *)&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY; // set local IP as any IP address
        sockaddr.sin_port = htons(port_);
        if ((bind(sockfd_, (struct sockaddr*)&sockaddr, sizeof(sockaddr))) < 0) {
            std::ostringstream oss;
            oss << "failed to bind UDP socket on port " << port_ << " (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
}






/*#include <arpa/inet.h> // inetaddr conversion; endianess conversion
#include <netinet/tcp.h> // TCP_NODELAY

// udp

void udpsendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen, const char* role) {
	int res = sendto(sockfd, buf, len, flags, (struct sockaddr *)dest_addr, addrlen);
	if (res < 0) {
		printf("[%s] sendto of udp socket fails, errno: %d!\n", role, errno);
		exit(-1);
	}
}

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
}

// TODO: NOT supported yet
//bool udprecvlarge_multisrc_udpfrag(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, bool isfilter, optype_t optype, netreach_key_t targetkey) {
//	return udprecvlarge_multisrc(methodid, sockfd, bufs_ptr, bufnum, flags, src_addrs, addrlens, role, UDP_FRAGMENT_MAXSIZE, UDP_FRAGTYPE, isfilter, optype, targetkey);
//}

bool udprecvlarge_multisrc_ipfrag(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, bool isfilter, optype_t optype, netreach_key_t targetkey) {
	return udprecvlarge_multisrc(methodid, sockfd, bufs_ptr, bufnum, flags, src_addrs, addrlens, role, IP_FRAGMENT_MAXSIZE, IP_FRAGTYPE, isfilter, optype, targetkey);
}

// NOTE: receive large packet from multiple sources; used for SCANRES_SPLIT (IMPORTANT: srcid > 0 && srcid <= srcnum <= bufnum)
// *bufs_ptr: max_srcnum dynamic arrays; if is_timeout = true, *bufs_ptr = NULL;
// bufnum: max_srcnum; if is_timeout = true, bufnum = 0;
// src_addrs: NULL or bufnum; addrlens: NULL or bufnum
// For example, srcnum for SCANRES_SPLIT is split_hdr.max_scannum; srcid for SCANRES_SPLIT is split_hdr.cur_scanidx
bool udprecvlarge_multisrc(method_t methodid, int sockfd, dynamic_array_t **bufs_ptr, size_t &bufnum, int flags, struct sockaddr_in *src_addrs, socklen_t *addrlens, const char* role, size_t frag_maxsize, int fragtype, bool isfilter, optype_t optype, netreach_key_t targetkey) {
	INVARIANT(fragtype == IP_FRAGTYPE);

	bool isfirst = true;
	size_t frag_hdrsize = 0;
	size_t srcnum_off = 0;
	size_t srcnum_len = 0;
	bool srcnum_conversion = false;
	size_t srcid_off = 0;
	size_t srcid_len = 0;
	bool srcid_conversion = false;

	size_t final_frag_hdrsize = 0;
	size_t frag_bodysize = 0;

	INVARIANT(Packet<key_t>::is_singleswitch(methodid));

	bool is_timeout = false;
	struct sockaddr_in tmp_srcaddr;
	socklen_t tmp_addrlen;

	// receive current fragment
	char fragbuf[frag_maxsize];
	int frag_recvsize = 0;
	// set by the global first fragment
	bool global_isfirst = true;
	size_t max_srcnum = 0;
	size_t cur_srcnum = 0;
	// set by the first fragment of each srcid
	bool *local_isfirsts = NULL;
	uint16_t *max_fragnums = NULL;
	uint16_t *cur_fragnums = NULL;
	netreach_key_t tmpkey;
	while (true) {
		is_timeout = udprecvfrom(sockfd, fragbuf, frag_maxsize, flags, &tmp_srcaddr, &tmp_addrlen, frag_recvsize, role);
		if (is_timeout) {
			break;
		}

		packet_type_t tmptype = get_packet_type(fragbuf, frag_recvsize);
		if (isfilter) {
			if (optype_t(tmptype) != optype) {
				continue; // filter the unmatched packet
			}
			tmpkey = get_packet_key(methodid, fragbuf, frag_recvsize);
			if (tmpkey != targetkey) {
				continue;
			}
		}

		if (isfirst) {
			frag_hdrsize = get_frag_hdrsize(methodid, tmptype);
			srcnum_off = get_srcnum_off(methodid, tmptype);
			srcnum_len = get_srcnum_len(tmptype);
			srcnum_conversion = get_srcnum_conversion(tmptype);
			srcid_off = get_srcid_off(methodid, tmptype);
			srcid_len = get_srcid_len(tmptype);
			srcid_conversion = get_srcid_conversion(tmptype);

			final_frag_hdrsize = frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
			frag_bodysize = frag_maxsize - final_frag_hdrsize;
			isfirst = false;

			INVARIANT(srcnum_len == 1 || srcnum_len == 2 || srcnum_len == 4);
			INVARIANT(srcid_len == 1 || srcid_len == 2 || srcid_len == 4);
			INVARIANT(srcnum_off <= frag_hdrsize && srcid_off <= frag_hdrsize);
		}

		INVARIANT(size_t(frag_recvsize) >= final_frag_hdrsize);

		// set max_srcnum for the global first packet
		if (global_isfirst) {
			//printf("frag_hdrsize: %d, final_frag_hdrsize: %d, frag_maxsize: %d, frag_bodysize: %d\n", frag_hdrsize, final_frag_hdrsize, frag_maxsize, frag_bodysize);
			memcpy(&max_srcnum, fragbuf + srcnum_off, srcnum_len);
			if (srcnum_conversion && srcnum_len == 2) max_srcnum = size_t(ntohs(uint16_t(max_srcnum)));
			else if (srcnum_conversion && srcnum_len == 4) max_srcnum = size_t(ntohl(uint32_t(max_srcnum)));

			// initialize
			bufnum = max_srcnum;
			*bufs_ptr = new dynamic_array_t[max_srcnum];
			local_isfirsts = new bool[max_srcnum];
			max_fragnums = new uint16_t[max_srcnum];
			cur_fragnums = new uint16_t[max_srcnum];
			for (size_t i = 0; i < max_srcnum; i++) {
				(*bufs_ptr)[i].init(MAX_BUFSIZE, MAX_LARGE_BUFSIZE);
				local_isfirsts[i] = true;
				max_fragnums[i] = 0;
				cur_fragnums[i] = 0;
			}

			global_isfirst = false;
		}

		uint16_t tmpsrcid = 0;
		memcpy(&tmpsrcid, fragbuf + srcid_off, srcid_len);
		if (srcid_conversion && srcid_len == 2) tmpsrcid = size_t(ntohs(uint16_t(tmpsrcid)));
		else if (srcid_conversion && srcid_len == 4) tmpsrcid = size_t(ntohl(uint32_t(tmpsrcid)));
		//printf("tmpsrcid: %d, max_srcnum: %d, bufnum: %d\n", tmpsrcid, max_srcnum, bufnum);
		INVARIANT(tmpsrcid > 0 && tmpsrcid <= max_srcnum);

		int tmp_bufidx = tmpsrcid - 1; // [1, max_srcnum] -> [0, max_srcnum-1]
		dynamic_array_t &tmpbuf = (*bufs_ptr)[tmp_bufidx];

		if (local_isfirsts[tmp_bufidx]) {
			if (src_addrs != NULL) {
				src_addrs[tmp_bufidx] = tmp_srcaddr;
				addrlens[tmp_bufidx] = tmp_addrlen;
			}

			tmpbuf.dynamic_memcpy(0, fragbuf, frag_hdrsize);
			memcpy(&max_fragnums[tmp_bufidx], fragbuf + frag_hdrsize + sizeof(uint16_t), sizeof(uint16_t));
			max_fragnums[tmp_bufidx] = ntohs(max_fragnums[tmp_bufidx]); // bigendian -> littleendian for large value

			local_isfirsts[tmp_bufidx] = false;
		}

		uint16_t cur_fragidx = 0;
		memcpy(&cur_fragidx, fragbuf + frag_hdrsize, sizeof(uint16_t));
		cur_fragidx = ntohs(cur_fragidx); // bigendian -> littleendian for large value
		INVARIANT(cur_fragidx < max_fragnums[tmp_bufidx]);
		//printf("cur_fragidx: %d, max_fragnum: %d, frag_recvsize: %d, buf_offset: %d, copy_size: %d\n", cur_fragidx, max_fragnums[tmp_bufidx], frag_recvsize, cur_fragidx * frag_bodysize, frag_recvsize - final_frag_hdrsize);

		tmpbuf.dynamic_memcpy(frag_hdrsize + cur_fragidx * frag_bodysize, fragbuf + final_frag_hdrsize, frag_recvsize - final_frag_hdrsize);

		cur_fragnums[tmp_bufidx] += 1;
		INVARIANT(cur_fragnums[tmp_bufidx] <= max_fragnums[tmp_bufidx]);
		if (cur_fragnums[tmp_bufidx] == max_fragnums[tmp_bufidx]) {
#ifdef SERVER_ROTATION
			break; // return as long as receiving all fragments of the ScanResponseSplit from one server for server rotation
#else
			cur_srcnum += 1;
			if (cur_srcnum >= max_srcnum) {
				break;
			}
#endif
		}
	}

	INVARIANT(cur_srcnum == bufnum && bufnum == max_srcnum);
	if (local_isfirsts != NULL) {
		delete [] local_isfirsts;
		local_isfirsts = NULL;
	}
	if (max_fragnums != NULL) {
		delete [] max_fragnums;
		max_fragnums = NULL;
	}
	if (cur_fragnums != NULL) {
		delete [] cur_fragnums;
		cur_fragnums = NULL;
	}

	if (is_timeout) {
		if ((*bufs_ptr) != NULL) {
			delete [] (*bufs_ptr);
			*bufs_ptr = NULL;
		}
		bufnum = 0;
	}

	INVARIANT(bufnum == 0 || (bufnum > 0 && (*bufs_ptr) != NULL));

	return is_timeout;
}

// TODO: NOT supported yet
//bool udprecvlarge_multisrc_udpfrag_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, bool isfilter, optype_t optype, netreach_key_t targetkey) {
//	return udprecvlarge_multisrc_dist(methodid, sockfd, perswitch_perserver_bufs, flags, perswitch_perserver_addrs, perswitch_perserver_addrlens, role, UDP_FRAGMENT_MAXSIZE, UDP_FRAGTYPE, isfilter, optype, targetkey);
//}

bool udprecvlarge_multisrc_ipfrag_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, bool isfilter, optype_t optype, netreach_key_t targetkey) {
	return udprecvlarge_multisrc_dist(methodid, sockfd, perswitch_perserver_bufs, flags, perswitch_perserver_addrs, perswitch_perserver_addrlens, role, IP_FRAGMENT_MAXSIZE, IP_FRAGTYPE, isfilter, optype, targetkey);
}

// NOTE: receive large packet from multiple sources; used for SCANRES_SPLIT
// IMPORTANT: srcswitchid > 0 && srcswitchid <= srcswitchnum; srcid > 0 && srcid <= srcnum; each srcswitchid can have different srcnums
// perswitch_perserver_bufs: srcswitchnum * srcnum dynamic arrays; if is_timeout = true, size = 0
// perswitch_perserver_addrs/addrlens have the same shape as perswitch_perserver_bufs
// For example, for SCANRES_SPLIT, srcswitchnum is split_hdr.max_scanswitchnum; srcswitchid is split_hdr.cur_scanswitchidx; srcnum is split_hdr.max_scannum; srcid is split_hdr.cur_scanidx
bool udprecvlarge_multisrc_dist(method_t methodid, int sockfd, std::vector<std::vector<dynamic_array_t>> &perswitch_perserver_bufs, int flags, std::vector<std::vector<struct sockaddr_in>> &perswitch_perserver_addrs, std::vector<std::vector<socklen_t>> &perswitch_perserver_addrlens, const char* role, size_t frag_maxsize, int fragtype, bool isfilter, optype_t optype, netreach_key_t targetkey) {
	INVARIANT(fragtype == IP_FRAGTYPE);

	bool isfirst = true;
	size_t frag_hdrsize = 0;
	size_t srcnum_off = 0;
	size_t srcnum_len = 0;
	bool srcnum_conversion = false;
	size_t srcid_off = 0;
	size_t srcid_len = 0;
	bool srcid_conversion = false;
	size_t srcswitchnum_off = 0;
	size_t srcswitchnum_len = 0;
	bool srcswitchnum_conversion = false;
	size_t srcswitchid_off = 0;
	size_t srcswitchid_len = 0;
	bool srcswitchid_conversion = false;

	size_t final_frag_hdrsize = 0;
	size_t frag_bodysize = 0;

	INVARIANT(!Packet<key_t>::is_singleswitch(methodid));

	bool is_timeout = false;
	struct sockaddr_in tmp_srcaddr;
	socklen_t tmp_addrlen;

	// receive current fragment
	char fragbuf[frag_maxsize];
	int frag_recvsize = 0;
	// set by the global first fragment
	bool global_isfirst = true;
	size_t cur_srcswitchnum = 0; // already received switchnum
	std::vector<size_t> perswitch_cur_srcnums; // per-switch already received servernum
	// set by the first fragment of each srcswitchid and each srcid
	std::vector<std::vector<uint16_t>> perswitch_perserver_max_fragnums;
	std::vector<std::vector<uint16_t>> perswitch_perserver_cur_fragnums; // per-switch per-server already received fragnum
	netreach_key_t tmpkey;
	while (true) {
		is_timeout = udprecvfrom(sockfd, fragbuf, frag_maxsize, flags, &tmp_srcaddr, &tmp_addrlen, frag_recvsize, role);
		if (is_timeout) {
			break;
		}

		packet_type_t tmptype = get_packet_type(fragbuf, frag_recvsize);
		if (isfilter) {
			//printf("received optype: %x, expected optype: %x\n", int(get_packet_type(fragbuf, frag_recvsize)), int(optype));
			if (optype_t(tmptype) != optype) {
				continue; // filter the unmatched packet
			}
			tmpkey = get_packet_key(methodid, fragbuf, frag_recvsize);
			//printf("received key: %x, expected key: %x\n", tmpkey.keyhihi, targetkey.keyhihi);
			if (tmpkey != targetkey) {
				continue;
			}
		}

		if (isfirst) {
			frag_hdrsize = get_frag_hdrsize(methodid, tmptype);
			srcnum_off = get_srcnum_off(methodid, tmptype);
			srcnum_len = get_srcnum_len(tmptype);
			srcnum_conversion = get_srcnum_conversion(tmptype);
			srcid_off = get_srcid_off(methodid, tmptype);
			srcid_len = get_srcid_len(tmptype);
			srcid_conversion = get_srcid_conversion(tmptype);
			srcswitchnum_off = get_srcswitchnum_off(methodid, tmptype);
			srcswitchnum_len = get_srcswitchnum_len(tmptype);
			srcswitchnum_conversion = get_srcswitchnum_conversion(tmptype);
			srcswitchid_off = get_srcswitchid_off(methodid, tmptype);
			srcswitchid_len = get_srcswitchid_len(tmptype);
			srcswitchid_conversion = get_srcswitchid_conversion(tmptype);

			final_frag_hdrsize = frag_hdrsize + sizeof(uint16_t) + sizeof(uint16_t);
			frag_bodysize = frag_maxsize - final_frag_hdrsize;

			INVARIANT(srcnum_len == 1 || srcnum_len == 2 || srcnum_len == 4);
			INVARIANT(srcid_len == 1 || srcid_len == 2 || srcid_len == 4);
			INVARIANT(srcnum_off <= frag_hdrsize && srcid_off <= frag_hdrsize);
			INVARIANT(srcswitchnum_len == 1 || srcswitchnum_len == 2 || srcswitchnum_len == 4);
			INVARIANT(srcswitchid_len == 1 || srcswitchid_len == 2 || srcswitchid_len == 4);
			INVARIANT(srcswitchnum_off <= frag_hdrsize && srcswitchid_off <= frag_hdrsize);

			isfirst = false;
		}

		INVARIANT(size_t(frag_recvsize) >= final_frag_hdrsize);


		if (global_isfirst) { // first packet in global -> get switchnum
			size_t max_srcswitchnum = 0;
			memcpy(&max_srcswitchnum, fragbuf + srcswitchnum_off, srcswitchnum_len);
			if (srcswitchnum_conversion && srcswitchnum_len == 2) max_srcswitchnum = size_t(ntohs(uint16_t(max_srcswitchnum)));
			else if (srcswitchnum_conversion && srcswitchnum_len == 4) max_srcswitchnum = size_t(ntohl(uint32_t(max_srcswitchnum)));
			INVARIANT(max_srcswitchnum > 0);

			// initialize
			perswitch_perserver_bufs.resize(max_srcswitchnum);
			perswitch_perserver_addrs.resize(max_srcswitchnum);
			perswitch_perserver_addrlens.resize(max_srcswitchnum);
			perswitch_cur_srcnums.resize(max_srcswitchnum, 0);
			perswitch_perserver_max_fragnums.resize(max_srcswitchnum);
			perswitch_perserver_cur_fragnums.resize(max_srcswitchnum);

			global_isfirst = false;

			//printf("frag_hdrsize: %d, final_frag_hdrsize: %d, frag_maxsize: %d, frag_bodysize: %d, max_srcswitchnum: %d\n", frag_hdrsize, final_frag_hdrsize, frag_maxsize, frag_bodysize, max_srcswitchnum);
		}

		// get switchidx
		uint16_t tmpsrcswitchid = 0;
		memcpy(&tmpsrcswitchid, fragbuf + srcswitchid_off, srcswitchid_len);
		if (srcswitchid_conversion && srcswitchid_len == 2) tmpsrcswitchid = size_t(ntohs(uint16_t(tmpsrcswitchid)));
		else if (srcswitchid_conversion && srcswitchid_len == 4) tmpsrcswitchid = size_t(ntohl(uint32_t(tmpsrcswitchid)));
		INVARIANT(tmpsrcswitchid > 0 && tmpsrcswitchid <= perswitch_perserver_bufs.size());
		int tmp_switchidx = tmpsrcswitchid - 1; // [1, max_srcswitchnum] -> [0, max_srcswitchnum-1]

		//printf("tmpsrcswitchid: %d, tmp_switchidx: %d\n", tmpsrcswitchid, tmp_switchidx);

		if (perswitch_perserver_bufs[tmp_switchidx].size() == 0) { // first packet from the leaf switch -> get servernum for the switch
			size_t max_srcnum = 0;
			memcpy(&max_srcnum, fragbuf + srcnum_off, srcnum_len);
			if (srcnum_conversion && srcnum_len == 2) max_srcnum = size_t(ntohs(uint16_t(max_srcnum)));
			else if (srcnum_conversion && srcnum_len == 4) max_srcnum = size_t(ntohl(uint32_t(max_srcnum)));
			INVARIANT(max_srcnum > 0);

			// initialize
			perswitch_perserver_bufs[tmp_switchidx].resize(max_srcnum);
			for (size_t i = 0; i < max_srcnum; i++) {
				perswitch_perserver_bufs[tmp_switchidx][i].init(MAX_BUFSIZE, MAX_LARGE_BUFSIZE);
			}
			perswitch_perserver_addrs[tmp_switchidx].resize(max_srcnum);
			perswitch_perserver_addrlens[tmp_switchidx].resize(max_srcnum, sizeof(struct sockaddr_in));
			perswitch_perserver_max_fragnums[tmp_switchidx].resize(max_srcnum, 0);
			perswitch_perserver_cur_fragnums[tmp_switchidx].resize(max_srcnum, 0);

			//printf("max_srcnum: %d\n", max_srcnum);
		}

		// get serveridx
		uint16_t tmpsrcid = 0;
		memcpy(&tmpsrcid, fragbuf + srcid_off, srcid_len);
		if (srcid_conversion && srcid_len == 2) tmpsrcid = size_t(ntohs(uint16_t(tmpsrcid)));
		else if (srcid_conversion && srcid_len == 4) tmpsrcid = size_t(ntohl(uint32_t(tmpsrcid)));
		//printf("tmpsrcid: %d, max_srcnum: %d, bufnum: %d\n", tmpsrcid, max_srcnum, bufnum);
		INVARIANT(tmpsrcid > 0 && tmpsrcid <= perswitch_perserver_bufs[tmp_switchidx].size());
		int tmp_bufidx = tmpsrcid - 1; // [1, max_srcnum] -> [0, max_srcnum-1]

		//printf("tmpsrcid: %d, tmp_bufidx: %d\n", tmpsrcid, tmp_bufidx);

		// get dynamic array for the switch and the server
		dynamic_array_t &tmpbuf = perswitch_perserver_bufs[tmp_switchidx][tmp_bufidx];

		if (perswitch_perserver_max_fragnums[tmp_switchidx][tmp_bufidx] == 0) { // first packet from the leaf switch and the server
			uint16_t max_fragnum = 0;
			memcpy(&max_fragnum, fragbuf + frag_hdrsize + sizeof(uint16_t), sizeof(uint16_t));
			max_fragnum = ntohs(max_fragnum); // bigendian -> littleendian for large value
			INVARIANT(max_fragnum > 0);

			perswitch_perserver_addrs[tmp_switchidx][tmp_bufidx] = tmp_srcaddr;
			perswitch_perserver_addrlens[tmp_switchidx][tmp_bufidx] = tmp_addrlen;
			perswitch_perserver_max_fragnums[tmp_switchidx][tmp_bufidx] = max_fragnum;

			tmpbuf.dynamic_memcpy(0, fragbuf, frag_hdrsize);

			//printf("max_fragnum: %d\n", max_fragnum);
		}

		uint16_t cur_fragidx = 0;
		memcpy(&cur_fragidx, fragbuf + frag_hdrsize, sizeof(uint16_t));
		cur_fragidx = ntohs(cur_fragidx); // bigendian -> littleendian for large value
		INVARIANT(cur_fragidx < perswitch_perserver_max_fragnums[tmp_switchidx][tmp_bufidx]);
		//printf("cur_fragidx: %d, max_fragnum: %d, frag_recvsize: %d, buf_offset: %d, copy_size: %d\n", cur_fragidx, max_fragnums[tmp_bufidx], frag_recvsize, cur_fragidx * frag_bodysize, frag_recvsize - final_frag_hdrsize);

		tmpbuf.dynamic_memcpy(frag_hdrsize + cur_fragidx * frag_bodysize, fragbuf + final_frag_hdrsize, frag_recvsize - final_frag_hdrsize);

		//printf("cur_fragidx: %d\n", cur_fragidx);

		perswitch_perserver_cur_fragnums[tmp_switchidx][tmp_bufidx] += 1;
		INVARIANT(perswitch_perserver_cur_fragnums[tmp_switchidx][tmp_bufidx] <= perswitch_perserver_max_fragnums[tmp_switchidx][tmp_bufidx]);
		if (perswitch_perserver_cur_fragnums[tmp_switchidx][tmp_bufidx] == perswitch_perserver_max_fragnums[tmp_switchidx][tmp_bufidx]) {
#ifdef SERVER_ROTATION
			break; // return as long as receiving all fragments of the ScanResponseSplit from one server under one switch for server rotation
#else
			perswitch_cur_srcnums[tmp_switchidx] += 1;
			if (perswitch_cur_srcnums[tmp_switchidx] >= perswitch_perserver_bufs[tmp_switchidx].size()) {
				cur_srcswitchnum += 1;
				if (cur_srcswitchnum >= perswitch_perserver_bufs.size()) {
					break;
				}
			}
#endif
		}
	}

	if (is_timeout) {
		perswitch_perserver_bufs.clear();
		perswitch_perserver_addrs.clear();
		perswitch_perserver_addrlens.clear();
	}

	return is_timeout;
}*/