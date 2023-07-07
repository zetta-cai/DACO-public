#include "network/udp_msg_socket_client.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/udp_frag_hdr.h"

namespace covered
{
    const bool UdpMsgSocketClient::IS_SOCKET_CLIENT_TIMEOUT(true); // timeout-and-retry for UDP client

	const std::string UdpMsgSocketClient::kClassName("UdpMsgSocketClient");

	UdpMsgSocketClient::UdpMsgSocketClient()
    {
		// Create UdpPktSocket for UDP client
		pkt_socket_ptr_ = new UdpPktSocket(IS_SOCKET_CLIENT_TIMEOUT);
		if (pkt_socket_ptr_ == NULL)
		{
			Util::dumpErrorMsg(kClassName, "failed to create UdpPktSocket for UDP client!");
			exit(1);
		}

		msg_seqnum_ = 0;
    }

	UdpMsgSocketClient::~UdpMsgSocketClient()
	{
		assert(pkt_socket_ptr_ != NULL);
		delete pkt_socket_ptr_;
		pkt_socket_ptr_ = NULL;
	}

    void UdpMsgSocketClient::send(MessageBase* message_ptr, const NetworkAddr& remote_addr)
	{
		assert(message_ptr != NULL);

		// Must with valid remote address
		assert(remote_addr.isValidAddr());
		assert(remote_addr.getIpstr() != Util::ANY_IPSTR);

		// Will be used by UdpMsgSocketServer to track message fragments for each source address
		NetworkAddr source_addr = message_ptr->getSourceAddr();

		// Prepare message payload
		uint32_t message_payload_size = message_ptr->getMsgPayloadSize();
		DynamicArray message_payload(message_payload_size);
		uint32_t serialize_size = message_ptr->serialize(message_payload);
		assert(serialize_size == message_payload_size);

		// Split message payload into multiple fragment payloads
		uint32_t fragment_cnt = Util::getFragmentCnt(message_payload_size);
		for (uint32_t fragment_idx = 0; fragment_idx < fragment_cnt; fragment_idx++)
		{
			// Prepare packet payload for current UDP packet
			DynamicArray tmp_pkt_payload(Util::UDP_MAX_PKT_PAYLOAD);

			// Serialize fragment header into current UDP packet
			UdpFragHdr fraghdr(fragment_idx, fragment_cnt, message_payload_size, msg_seqnum_, source_addr);
			uint32_t fraghdr_size = fraghdr.serialize(tmp_pkt_payload);

			// Calculate fragment offset and size in message based on fragment index			
			uint32_t fragment_offset = Util::getFragmentOffset(fragment_idx);
			uint32_t fragment_payload_size = Util::getFragmentPayloadSize(fragment_idx, message_payload_size);

			// Copy UDP fragment payload into current UDP packet
			message_payload.arraycpy(fragment_offset, tmp_pkt_payload, fraghdr_size, fragment_payload_size);

			// Send current UDP packet by UdpPktSocket
			pkt_socket_ptr_->udpSendto(tmp_pkt_payload, remote_addr);
		}

		msg_seqnum_ += 1;
		return;
	}
}