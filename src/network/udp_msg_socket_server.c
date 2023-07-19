#include "network/udp_msg_socket_server.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "network/udp_frag_hdr.h"

namespace covered
{
    const bool UdpMsgSocketServer::IS_SOCKET_SERVER_TIMEOUT(true); // timeout for UDP server to safely terminate

	const std::string UdpMsgSocketServer::kClassName("UdpMsgSocketServer");

    UdpMsgSocketServer::UdpMsgSocketServer(const NetworkAddr& host_addr) : msg_frag_stats_()
    {
		assert(host_addr.isValidAddr());

		// Not support to bind a specific host IP for UDP server
		std::string host_ipstr = host_addr.getIpstr();
		if (host_ipstr != Util::ANY_IPSTR)
		{
			std::ostringstream oss;
			oss << "NOT support a specific host ipstr of " << host_ipstr << " for UDP server now!";	
			Util::dumpErrorMsg(kClassName, oss.str());
			exit(1);
		}

		// Create UdpPktSocket for UDP server
		pkt_socket_ptr_ = new UdpPktSocket(IS_SOCKET_SERVER_TIMEOUT, host_addr);
		if (pkt_socket_ptr_ == NULL)
		{
			Util::dumpErrorMsg(kClassName, "failed to create UdpPktSocket for UDP server!");
			exit(1);
		}
    }

	UdpMsgSocketServer::~UdpMsgSocketServer()
	{
		assert(pkt_socket_ptr_ != NULL);
		delete pkt_socket_ptr_;
		pkt_socket_ptr_ = NULL;
	}

	bool UdpMsgSocketServer::recv(DynamicArray& msg_payload)
	{
		bool is_timeout = false;

		while (true) // Until receive a complete message
		{
			// Prepare to receive a UDP packet
			NetworkAddr tmp_propagation_simulator_addr;
			DynamicArray tmp_pkt_payload(Util::UDP_MAX_PKT_PAYLOAD);

			is_timeout = pkt_socket_ptr_->udpRecvfrom(tmp_pkt_payload, tmp_propagation_simulator_addr);
			UNUSED(tmp_propagation_simulator_addr);
			if (is_timeout == true) // timeout (not receive any UDP packet)
			{
				break;
			}
			else // not timeout (receive a UDP packet)
			{
				// Deserialize fragment header from currently received packet payload
				UdpFragHdr fraghdr(tmp_pkt_payload);
				NetworkAddr source_addr = fraghdr.getSourceAddr();

				// Use MsgFragStats to track fragment statistics of each message
				bool is_last_frag = msg_frag_stats_.insertEntry(source_addr, tmp_pkt_payload);
				if (is_last_frag == true) // All fragment(s) of the message have been received
				{
					// Prepare message payload for current message
					uint32_t msg_payload_size = fraghdr.getMsgPayloadSize();
					msg_payload.clear(msg_payload_size);

					// Copy previously received fragment payloads if any
					uint32_t fragcnt = fraghdr.getFragmentCnt();
					if (fragcnt > 1) // more than 1 fragments -> MsgFragStatsEntry exists
					{
						// Get previously received fragment payloads
						MsgFragStatsEntry* msg_frag_stats_entry = msg_frag_stats_.getEntry(source_addr);
						assert(msg_frag_stats_entry != NULL);
						std::map<uint32_t, DynamicArray>& fragidx_fragpayload_map = msg_frag_stats_entry->getFragidxFragpayloadMapRef();

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
						msg_frag_stats_.removeEntry(source_addr);
					} // End of (fragcnt > 1)

					// Copy currently received packet payload into current message payload
					uint32_t fragidx = fraghdr.getFragmentIdx();
					uint32_t fragment_offset = Util::getFragmentOffset(fragidx);
					uint32_t fragment_payload_size = Util::getFragmentPayloadSize(fragidx, msg_payload_size);
					tmp_pkt_payload.arraycpy(Util::UDP_FRAGHDR_SIZE, msg_payload, fragment_offset, fragment_payload_size);

					// Update network address for processing outside UdpSocketWrapper
					//network_addr = source_addr;
					//assert(network_addr.isValidAddr() == true);

					break; // Break while(true)
				} // End of (is_last_frag == true)
			} // End of (is_timeout == false)
		} // End of while(true)

		return is_timeout;
	}
}