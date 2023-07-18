/*
 * MsgFragStats: track fragment statistics of each message to cope with packet reordering especially for large messages.
 * 
 * By Siyuan Sheng (2023.04.25).
 */

#ifndef MSG_FRAG_STATS_H
#define MSG_FRAG_STATS_H

#include <map>
#include <string>

#include "common/dynamic_array.h"
#include "network/network_addr.h"
#include "network/udp_frag_hdr.h"

namespace covered
{
    class MsgFragStatsEntry
    {
    public:
        MsgFragStatsEntry();
        ~MsgFragStatsEntry();

        bool isLastFrag(const UdpFragHdr& fraghdr);
        void insertFrag(const UdpFragHdr& fraghdr, const DynamicArray& pkt_payload);
        std::map<uint32_t, DynamicArray>& getFragidxFragpayloadMapRef();
    private:
        static const std::string kClassName;

        std::map<uint32_t, DynamicArray> fragidx_fragpayload_map_;
        uint32_t fragcnt_;
        uint32_t msg_seqnum_;
    };

    class MsgFragStats
    {
    public:
        MsgFragStats();
        ~MsgFragStats();

        bool insertEntry(const NetworkAddr& source_addr, const DynamicArray& pkt_payload); // Return if the current fragment is the last one (must be true for fragcnt of 1)
        MsgFragStatsEntry* getEntry(const NetworkAddr& source_addr);
        void removeEntry(const NetworkAddr& source_addr);
    private:
        static const std::string kClassName;

        // NOTE: we should use source address in UDP payload instead of propagation simulator's address in UDP header (e.g., different client workers or cache server workers will share the same propagation simulator's address)
        std::map<NetworkAddr, MsgFragStatsEntry> srcaddr_entry_map_;
    };
}

#endif
