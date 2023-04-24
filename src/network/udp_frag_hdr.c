#include "udp_fragment_header.h"

#include <cstring> // memcpy
#include <arpa/inet.h> // htonl ntohl

#include "common/util.h"

namespace covered
{
    const std::string UdpFragHdr::kClassName("UdpFragHdr");

    UdpFragHdr::UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt)
    {
        fragment_idx_ = fragment_idx;
        fragment_cnt_ = fragment_cnt;
    }

    ~UdpFragHdr() {}

    uint32_t serialize(std::vector<char>& pkt_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_fragment_idx = htonl(fragment_idx_);
        memcpy((void*)(pkt_payload.data()), (const void*)(&bigendian_fragment_idx), sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_fragment_cnt = htonl(fragment_cnt_);
        memcpy((void*)(pkt_payload.data() + size), (const void *)(&bigendian_fragment_cnt), sizeof(uint32_t));
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
        return size;
    }
}