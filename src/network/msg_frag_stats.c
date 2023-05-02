#include "msg_frag_stats.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    // MsgFragStatsEntry

    const std::string MsgFragStatsEntry::kClassName("MsgFragStatsEntry");

    MsgFragStatsEntry::MsgFragStatsEntry()
    {
        fragidx_fragpayload_map_.clear();
        fragcnt_ = 0;
        msg_seqnum_ = 0;
    }

    MsgFragStatsEntry::~MsgFragStatsEntry() {}

    bool MsgFragStatsEntry::isLastFrag(const UdpFragHdr& fraghdr)
    {
        uint32_t fragidx = fraghdr.getFragmentIdx();
        uint32_t seqnum = fraghdr.getMsgSeqnum();
        if (fragidx_fragpayload_map_.size() == fragcnt_ - 1 \
            && fragidx_fragpayload_map_.find(fragidx) == fragidx_fragpayload_map_.end() \
            && msg_seqnum_ == seqnum) // fragidx is the last untracked fragment with matched seqnum
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void MsgFragStatsEntry::insertFrag(const UdpFragHdr& fraghdr, const DynamicArray& pkt_payload)
    {
        uint32_t fragidx = fraghdr.getFragmentIdx();
        uint32_t fragcnt = fraghdr.getFragmentCnt();
        uint32_t seqnum = fraghdr.getMsgSeqnum();

        if (seqnum < msg_seqnum_)
        {
            return; // Ignore the fragment with smaller seqnum than tracked msg_seqnum_
        }
        else
        {            
            if (seqnum > msg_seqnum_) // Clear outdated fragments if the current fragment belongs to a retried message
            {
                fragidx_fragpayload_map_.clear();
                msg_seqnum_ = seqnum;
            }
            else // Fragments of the same message
            {
                assert(fragcnt == fragcnt_);
            }

            assert(seqnum == msg_seqnum_);
            if (fragidx_fragpayload_map_.find(fragidx) == fragidx_fragpayload_map_.end()) // Not tracked before
            {
                fragidx_fragpayload_map_.insert(std::pair<uint32_t, DynamicArray>(fragidx, DynamicArray(Util::UDP_MAX_FRAG_PAYLOAD)));
                DynamicArray& frag_payload = fragidx_fragpayload_map_[fragidx];
                pkt_payload.arraycpy(Util::UDP_FRAGHDR_SIZE, frag_payload, 0, pkt_payload.getSize() - Util::UDP_FRAGHDR_SIZE);
            }
        }

        return;
    }

    std::map<uint32_t, DynamicArray>& MsgFragStatsEntry::getFragidxFragpayloadMap()
    {
        return fragidx_fragpayload_map_;
    }

    // MsgFragStats

    const std::string MsgFragStats::kClassName("MsgFragStats");

    MsgFragStats::MsgFragStats()
    {
        addr_entry_map_.clear();
    }

    MsgFragStats::~MsgFragStats() {}

    // NOTE: we do NOT copy the last fragment into the entry to avoid one time of unnecessary memory copy
    bool MsgFragStats::insertEntry(const NetworkAddr& addr, const DynamicArray& pkt_payload)
    {
        assert(addr.isValid() == true);

        bool is_last_frag = false;

        UdpFragHdr fraghdr(pkt_payload);
        if (fraghdr.getFragmentCnt() == 1) // Must be the last fragment for fragcnt of 1
        {
            is_last_frag = true;
        }
        else
        {
            if (addr_entry_map_.find(addr) == addr_entry_map_.end()) // Not receive any fragment of the message before (must NOT be the last fragment)
            {
                addr_entry_map_.insert(std::pair<NetworkAddr, MsgFragStatsEntry>(addr, MsgFragStatsEntry()));
                addr_entry_map_[addr].insertFrag(fraghdr, pkt_payload);
            }
            else // Receive some fragments of the message before
            {
                MsgFragStatsEntry& entry = addr_entry_map_[addr];
                if (entry.isLastFrag(fraghdr)) // The last fragment of the message
                {
                    is_last_frag = true;
                }
                else // NOT the last fragment, which has to be tracked into the entry
                {
                    entry.insertFrag(fraghdr, pkt_payload);
                }
            }
        }
        return is_last_frag;
    }

    MsgFragStatsEntry* MsgFragStats::getEntry(const NetworkAddr& addr)
    {
        assert(addr.isValid() == true);
        
        if (addr_entry_map_.find(addr) == addr_entry_map_.end())
        {
            return NULL;
        }
        else
        {
            return &addr_entry_map_[addr];
        }
    }

    void MsgFragStats::removeEntry(const NetworkAddr& addr)
    {
        if (addr_entry_map_.find(addr) != addr_entry_map_.end())
        {
            addr_entry_map_.erase(addr);
        }
        return;
    }
}