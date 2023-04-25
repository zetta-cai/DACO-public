/*
 * MsgFragStats: track fragment statistics of each message to cope with packet reordering especially for large messages.
 * 
 * By Siyuan Sheng (2023.04.25).
 */

#ifndef MSG_FRAG_STATS_H
#define MSG_FRAG_STATS_H

#include <map>
#include <vector>
#include <string>

#include "network/network_addr.h"

namespace covered
{
    class MsgFragStatsEntry
    {
    public:
        MsgFragStatsEntry();
        ~MsgFragStatsEntry();

        bool isLastFrag(const UdpFragHdr& fraghdr);
        void insertFrag(const UdpFragHdr& fraghdr, const std::vector<char>& pkt_payload);
    private:
        static const std::string kClassName;

        std::map<uint32_t, std::vector<char>> fragidx_fragpayload_map_;
        uint32_t fragcnt_;
        uint32_t seqnum_;
    };

    class MsgFragStats
    {
    public:
        MsgFragStats();
        ~MsgFragStats();

        bool insertEntry(const NetworkAddr& addr, const std::vector<char>& pkt_payload); // Return if the current fragment is the last one (must be true for fragcnt of 1)
        MsgFragStatsEntry* getEntry(const NetworkAddr& addr);
        void removeEntry(const NetworkAddr& addr);
    private:
        static const std::string kClassName;

        std::map<NetworkAddr, MsgFragStatsEntry> addr_entry_map_;
    };
}

#endif
