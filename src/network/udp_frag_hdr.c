#include "udp_fragment_header.h"

#include <cstring> // memcpy
#include <arpa/inet.h> // htonl ntohl

#include "common/util.h"

namespace covered
{
    const std::string UdpFragHdr::kClassName("UdpFragHdr");

    UdpFragHdr::UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt, const uint32_t& msg_payload_size, const uint32_t& msg_seqnum)
    {
        fragment_idx_ = fragment_idx;
        fragment_cnt_ = fragment_cnt;
        msg_payload_size_ = msg_payload_size;
        msg_seqnum_ = msg_seqnum;
    }

    UdpFragHdr::UdpFragHdr(const std::vector<char>& pkt_payload)
    {
        deserialize(pkt_payload);
    }

    ~UdpFragHdr() {}

    uint32_t UdpFragHdr::getFragmentIdx()
    {
        return fragment_idx_;
    }

    uint32_t UdpFragHdr::getFragmentCnt()
    {
        return fragment_cnt_;
    }
    
    uint32_t UdpFragHdr::getMsgPayloadSize()
    {
        return msg_payload_size_;
    }

    uint32_t UdpFragHdr::getMsgSeqnum()
    {
        return msg_seqnum_;
    }

    uint32_t serialize(std::vector<char>& pkt_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_fragment_idx = htonl(fragment_idx_);
        memcpy((void*)(pkt_payload.data()), (const void*)(&bigendian_fragment_idx), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_fragment_cnt = htonl(fragment_cnt_);
        memcpy((void*)(pkt_payload.data() + size), (const void *)(&bigendian_fragment_cnt), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_payload_size = htonl(msg_payload_size_);
        memcpy((void*)(pkt_payload.data() + size), (const void *)(&bigendian_msg_payload_size), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_seqnum = htonl(msg_seqnum_);
        memcpy((void*)(pkt_payload.data() + size), (const void *)(&bigendian_msg_seqnum), sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size;
    }
    
    uint32_t deserialize(const std::vector<char>& pkt_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_fragment_idx = 0;
        memcpy((void*)(&bigendian_fragment_idx), (const void*)(pkt_payload.data()), sizeof(uint32_t));
        fragment_idx_ = ntohl(bigendian_fragment_idx);
        size += sizeof(uint32_t);
        uint32_t bigendian_fragment_cnt = 0;
        memcpy((void*)(&bigendian_fragment_cnt), (const void*)(pkt_payload.data() + size), sizeof(uint32_t));
        fragment_cnt_ = ntohl(bigendian_fragment_cnt);
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_payload_size = 0;
        memcpy((void*)(&bigendian_msg_payload_size), (const void*)(pkt_payload.data() + size), sizeof(uint32_t));
        msg_payload_size_ = ntohl(bigendian_msg_payload_size);
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_seqnum = 0;
        memcpy((void*)(&bigendian_msg_seqnum), (const void*)(pkt_payload.data() + size), sizeof(uint32_t));
        msg_seqnum_ = ntohl(bigendian_msg_seqnum);
        size += sizeof(uint32_t);
        return size;
    }
}