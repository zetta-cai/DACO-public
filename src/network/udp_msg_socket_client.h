/*
 * UdpMsgSocketClient: encapsulate advanced operations (UDP fragmentation of message payload) on UDP socket programming to send messages.
 *
 * NOTE: message payload may be splited into multiple UDP fragments (i.e., multiple fragment payloads), where each fragment payload + fragment header form the corresponding UDP packet payload.
 * 
 * NOTE: UdpMsgSocketClient is only used by PropagationSimulator to send messages to a given remote address.
 * 
 * By Siyuan Sheng (2023.07.02).
 */

#ifndef UDP_MSG_SOCKET_CLIENT_H
#define UDP_MSG_SOCKET_CLIENT_H

#include <string>

#include "message/message_base.h"
#include "network/network_addr.h"
#include "network/udp_pkt_socket.h"

namespace covered
{
    class UdpMsgSocketClient
    {
    public:
        // Whether to enable timeout for UDP client (only used in UdpMsgSocketClient)
        static const bool IS_SOCKET_CLIENT_TIMEOUT;

        UdpMsgSocketClient();
        ~UdpMsgSocketClient();

        void send(MessageBase* message_ptr, const NetworkAddr& remote_addr);
    private:
        static const std::string kClassName;

        UdpPktSocket* pkt_socket_ptr_; // send payload of each single UDP packet
        uint32_t msg_seqnum_;
    };
}

#endif