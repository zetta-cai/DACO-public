/*
 * UdpFragHdr: header fields for UDP fragmentation.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_FRAG_HDR_H
#define UDP_FRAG_HDR_H

#include <string>
#include <vector>

#include "common/dynamic_array.h"
#include "network/network_addr.h"

namespace covered
{
    class UdpFragHdr
    {
    public:
        UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt, const uint32_t& msg_payload_size, const uint32_t& msg_seqnum, const NetworkAddr& source_addr);
        UdpFragHdr(const DynamicArray& pkt_payload);
        ~UdpFragHdr();

        uint32_t getFragmentIdx() const;
        uint32_t getFragmentCnt() const;
        uint32_t getMsgPayloadSize() const;
        uint32_t getMsgSeqnum() const;
        NetworkAddr getSourceAddr() const;

        // Offset of UDP fragment header must be 0 in packet payload
        uint32_t serialize(DynamicArray& pkt_payload);
        uint32_t deserialize(const DynamicArray& pkt_payload);
    private:
        static const std::string kClassName;

        // NOTE: adding new fragment header fields needs to update Util::UDP_FRAGHDR_SIZE and serialize/deserialize
        uint32_t fragment_idx_;
        uint32_t fragment_cnt_;
        uint32_t msg_payload_size_;
        // TODO: use uint64_t seqnum to avoid sequence number overflow in MsgFragStatsEntry
        uint32_t msg_seqnum_; // To cope with duplicate packets
        NetworkAddr source_addr_;
    };
}

#endif