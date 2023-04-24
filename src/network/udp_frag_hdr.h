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
        UdpFragHdr(const uint32_t& fragment_idx, const uint32_t& fragment_cnt);
        ~UdpFragHdr();

        // Offset must be 0 for UDP fragment header
        uint32_t serialize(std::vector<char>& pkt_payload);
        uint32_t deserialize(const std::vector<char>& pkt_payload);
    private:
        static const std::string kClassName;

        // NOTE: adding new fragment header fields needs to update Util::UDP_MAX_FRAG_PAYLOAD and UdpFragHdr::serialize/deserialize
        uint32_t fragment_idx_;
        uint32_t fragment_cnt_;
    };
}

#endif