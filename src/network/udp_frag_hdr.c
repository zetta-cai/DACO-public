#include "network/udp_frag_hdr.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string UdpFragHdr::kClassName("UdpFragHdr");

    UdpFragHdr::UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt, const uint32_t& msg_payload_size, const uint32_t& msg_seqnum, const NetworkAddr& source_addr)
    {
        fragment_idx_ = fragment_idx;
        fragment_cnt_ = fragment_cnt;
        msg_payload_size_ = msg_payload_size;
        msg_seqnum_ = msg_seqnum;
        source_addr_ = source_addr;
    }

    UdpFragHdr::UdpFragHdr(const DynamicArray& pkt_payload)
    {
        deserialize(pkt_payload);
    }

    UdpFragHdr::~UdpFragHdr() {}

    uint32_t UdpFragHdr::getFragmentIdx() const
    {
        return fragment_idx_;
    }

    uint32_t UdpFragHdr::getFragmentCnt() const
    {
        return fragment_cnt_;
    }
    
    uint32_t UdpFragHdr::getMsgPayloadSize() const
    {
        return msg_payload_size_;
    }

    uint32_t UdpFragHdr::getMsgSeqnum() const
    {
        return msg_seqnum_;
    }

    NetworkAddr UdpFragHdr::getSourceAddr() const
    {
        return source_addr_;
    }

    uint32_t UdpFragHdr::serialize(DynamicArray& pkt_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_fragment_idx = htonl(fragment_idx_);
        pkt_payload.deserialize(size, (const char*)(&bigendian_fragment_idx), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_fragment_cnt = htonl(fragment_cnt_);
        pkt_payload.deserialize(size, (const char*)(&bigendian_fragment_cnt), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_payload_size = htonl(msg_payload_size_);
        pkt_payload.deserialize(size, (const char*)(&bigendian_msg_payload_size), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_seqnum = htonl(msg_seqnum_);
        pkt_payload.deserialize(size, (const char*)(&bigendian_msg_seqnum), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t addr_serialize_size = source_addr_.serialize(pkt_payload, size);
        size += addr_serialize_size;

        assert((size - 0) == Util::UDP_FRAGHDR_SIZE);
        return size - 0;
    }
    
    uint32_t UdpFragHdr::deserialize(const DynamicArray& pkt_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_fragment_idx = 0;
        pkt_payload.serialize(size, (char*)(&bigendian_fragment_idx), sizeof(uint32_t));
        fragment_idx_ = ntohl(bigendian_fragment_idx);
        size += sizeof(uint32_t);
        uint32_t bigendian_fragment_cnt = 0;
        pkt_payload.serialize(size, (char*)(&bigendian_fragment_cnt), sizeof(uint32_t));
        fragment_cnt_ = ntohl(bigendian_fragment_cnt);
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_payload_size = 0;
        pkt_payload.serialize(size, (char*)(&bigendian_msg_payload_size), sizeof(uint32_t));
        msg_payload_size_ = ntohl(bigendian_msg_payload_size);
        size += sizeof(uint32_t);
        uint32_t bigendian_msg_seqnum = 0;
        pkt_payload.serialize(size, (char*)(&bigendian_msg_seqnum), sizeof(uint32_t));
        msg_seqnum_ = ntohl(bigendian_msg_seqnum);
        size += sizeof(uint32_t);
        uint32_t addr_deserialize_size = source_addr_.deserialize(pkt_payload, size);
        size += addr_deserialize_size;

        assert((size - 0) == Util::UDP_FRAGHDR_SIZE);
        return size - 0;
    }
}