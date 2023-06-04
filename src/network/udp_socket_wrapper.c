#include "network/udp_socket_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "network/udp_frag_hdr.h"

namespace covered
{
    const bool UdpSocketWrapper::IS_SOCKET_CLIENT_TIMEOUT(true); // timeout-and-retry for UDP client
    const bool UdpSocketWrapper::IS_SOCKET_SERVER_TIMEOUT(true); // timeout for UDP server to safely terminate

    const std::string UdpSocketWrapper::kClassName("UdpSocketWrapper");

    UdpSocketWrapper::UdpSocketWrapper(const SocketRole& role, const NetworkAddr& addr) : role_(role), host_addr_(role == SocketRole::kSocketServer ? addr : NetworkAddr()), msg_frag_stats_(), msg_seqnum_(0)
    {
        if (role_ == SocketRole::kSocketClient)
        {
			// Not support broadcast for UDP client
			std::string remote_str = addr.getIpstr();
			if (remote_str == Util::ANY_IPSTR)
			{
				std::ostringstream oss;
				oss << "invalid remote ipstr of " << remote_str << " for UDP client!";	
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// Remote address is fixed to UDP client
			remote_addr_ = addr;

			// Create UdpPktSocket for UDP client
			pkt_socket_ptr_ = new UdpPktSocket(IS_SOCKET_CLIENT_TIMEOUT);
			if (pkt_socket_ptr_ == NULL)
			{
				Util::dumpErrorMsg(kClassName, "failed to create UdpPktSocket for UDP client!");
				exit(1);
			}

			assert(host_addr_.isValid() == false);
			assert(remote_addr_.isValid() == true);
        }
        else if (role_ == SocketRole::kSocketServer)
        {
			// Not support to bind a specific host IP for UDP server
			std::string host_ipstr = addr.getIpstr();
			if (host_ipstr != Util::ANY_IPSTR)
			{
				std::ostringstream oss;
				oss << "NOT support a specific host ipstr of " << host_ipstr << " for UDP server now!";	
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// Remote address is dynamically changed by recv for UDP server
			remote_addr_ = NetworkAddr();

			// Create UdpPktSocket for UDP server
			pkt_socket_ptr_ = new UdpPktSocket(IS_SOCKET_SERVER_TIMEOUT, host_addr_);
			if (pkt_socket_ptr_ == NULL)
			{
				Util::dumpErrorMsg(kClassName, "failed to create UdpPktSocket for UDP server!");
				exit(1);
			}

			assert(host_addr_.isValid() == true);
			assert(remote_addr_.isValid() == false);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid SocketRole: " << static_cast<int>(role_);
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

	UdpSocketWrapper::~UdpSocketWrapper()
	{
		assert(pkt_socket_ptr_ != NULL);
		delete pkt_socket_ptr_;
		pkt_socket_ptr_ = NULL;
	}

    void UdpSocketWrapper::send(const DynamicArray& msg_payload)
	{
		// Must with valid remote address
		if (remote_addr_.isValid() == false)
		{
			std::ostringstream oss;
            oss << "NO remote address for SocketRole: " << static_cast<int>(role_);
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
		}

		// Split message payload into multiple fragment payloads
		uint32_t fragment_cnt = Util::getFragmentCnt(msg_payload.getSize());
		for (uint32_t fragment_idx = 0; fragment_idx < fragment_cnt; fragment_idx++)
		{
			// Prepare packet payload for current UDP packet
			DynamicArray tmp_pkt_payload(Util::UDP_MAX_PKT_PAYLOAD);

			// Serialize fragment header into current UDP packet
			UdpFragHdr fraghdr(fragment_idx, fragment_cnt, msg_payload.getSize(), msg_seqnum_);
			uint32_t fraghdr_size = fraghdr.serialize(tmp_pkt_payload);

			// Calculate fragment offset and size in message based on fragment index			
			uint32_t fragment_offset = Util::getFragmentOffset(fragment_idx);
			uint32_t fragment_payload_size = Util::getFragmentPayloadSize(fragment_idx, msg_payload.getSize());

			// Copy UDP fragment payload into current UDP packet
			msg_payload.arraycpy(fragment_offset, tmp_pkt_payload, fraghdr_size, fragment_payload_size);

			// Send current UDP packet by UdpPktSocket
			pkt_socket_ptr_->udpSendto(tmp_pkt_payload, remote_addr_);
		}

		// Reset is_receive_remote_address for UDP server after sending all fragments
		if (role_ == SocketRole::kSocketServer)
		{
			// Mark remote address to be set by the next successful recv (i.e., receive all fragment payloads of a message payload)
			remote_addr_.resetValid();
		}
		msg_seqnum_ += 1;
		return;
	}

	bool UdpSocketWrapper::recv(DynamicArray& msg_payload)
	{
		bool is_timeout = false;

		while (true)
		{
			// Prepare to receive a UDP packet
			NetworkAddr tmp_addr;
			DynamicArray tmp_pkt_payload(Util::UDP_MAX_PKT_PAYLOAD);

			is_timeout = pkt_socket_ptr_->udpRecvfrom(tmp_pkt_payload, tmp_addr);
			if (is_timeout == true) // timeout (not receive any UDP packet)
			{
				break;
			}
			else // not timeout (receive a UDP packet)
			{
				assert(tmp_addr.isValid() == true);

				// Use MsgFragStats to track fragment statistics of each message
				bool is_last_frag = msg_frag_stats_.insertEntry(tmp_addr, tmp_pkt_payload);
				if (is_last_frag == true) // All fragment(s) of the message have been received
				{
					// Deserialize fragment header from currently received packet payload
					UdpFragHdr fraghdr(tmp_pkt_payload);

					// Prepare message payload for current message
					uint32_t msg_payload_size = fraghdr.getMsgPayloadSize();
					msg_payload.clear(msg_payload_size);
					uint32_t fragcnt = fraghdr.getFragmentCnt();
					if (fragcnt > 1) // more than 1 fragments -> MsgFragStatsEntry exists
					{
						// Get previously received fragment payloads
						MsgFragStatsEntry* msg_frag_stats_entry = msg_frag_stats_.getEntry(tmp_addr);
						assert(msg_frag_stats_entry != NULL);
						std::map<uint32_t, DynamicArray>& fragidx_fragpayload_map = msg_frag_stats_entry->getFragidxFragpayloadMap();

						// Copy previously received fragment payloads into message payload
						for (std::map<uint32_t, DynamicArray>::iterator iter = fragidx_fragpayload_map.begin(); iter != fragidx_fragpayload_map.end(); iter++)
						{
							// Calculate fragment offset and size in message based on fragment index
							uint32_t fragidx = iter->first;
							uint32_t fragment_offset = Util::getFragmentOffset(fragidx);
							uint32_t fragment_payload_size = Util::getFragmentPayloadSize(fragidx, msg_payload_size);

							// Copy previously received fragment payload into current message payload
							iter->second.arraycpy(0, msg_payload, fragment_offset, fragment_payload_size);
						}

						// Remove previously received fragment payloads
						msg_frag_stats_.removeEntry(tmp_addr);
					} // End of (fragcnt > 1)

					// Copy currently received packet payload into current message payload
					uint32_t fragidx = fraghdr.getFragmentIdx();
					uint32_t fragment_offset = Util::getFragmentOffset(fragidx);
					uint32_t fragment_payload_size = Util::getFragmentPayloadSize(fragidx, msg_payload_size);
					tmp_pkt_payload.arraycpy(Util::UDP_FRAGHDR_SIZE, msg_payload, fragment_offset, fragment_payload_size);

					// Update remote address for send in the near future
					remote_addr_ = tmp_addr;
					assert(remote_addr_.isValid() == true);

					break; // Break while(true)
				} // End of (is_last_frag == true)
			} // End of (is_timeout == false)
		} // End of while(true)

		return is_timeout;
	}
}