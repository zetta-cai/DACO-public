/*
 * UdpFragHdr: header fields for UDP fragmentation.
 * 
 * By Siyuan Sheng (2023.04.24).
 */

#ifndef UDP_FRAG_HDR_H
#define UDP_FRAG_HDR_H

#include <string>
#include <vector>

namespace covered
{
    class UdpFragHdr
    {
    public:
        UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt, const uint32_t& msg_payload_size, const uint32_t& msg_seqnum);
        UdpFragHdr(const std::vector<char>& pkt_payload);
        ~UdpFragHdr();

        uint32_t getFragmentIdx();
        uint32_t getFragmentCnt();
        uint32_t getMsgPayloadSize();
        uint32_t getMsgSeqnum();

        // Offset must be 0 for UDP fragment header
        uint32_t serialize(std::vector<char>& pkt_payload);
        uint32_t deserialize(const std::vector<char>& pkt_payload);
    private:
        static const std::string kClassName;

        // NOTE: adding new fragment header fields needs to update Util::UDP_FRAGHDR_SIZE and serialize/deserialize
        uint32_t fragment_idx_;
        uint32_t fragment_cnt_;
        uint32_t msg_payload_size_;
        // TODO: use uint64_t seqnum to avoid sequence number overflow in MsgFragStatsEntry
        uint32_T msg_seqnum_; // To cope with duplicate packets
    };
}

#endif