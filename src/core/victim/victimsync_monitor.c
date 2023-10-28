#include "core/victim/victimsync_monitor.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(nullptr), need_enforcement_(false), is_first_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false)
    {
        cached_victim_syncsets_.clear();
    }

    VictimsyncMonitor::~VictimsyncMonitor()
    {
        if (prev_victim_syncset_ptr_ != NULL)
        {
            delete prev_victim_syncset_ptr_;
            prev_victim_syncset_ptr_ = NULL;
        }
    }

    // As sender edge node
    
    SeqNum VictimsyncMonitor::getCurSeqnum() const
    {
        return cur_seqnum_;
    }

    void VictimsyncMonitor::incrCurSeqnum()
    {
        cur_seqnum_ = Util::uint64Add(cur_seqnum_, 1);
        return;
    }

    bool VictimsyncMonitor::getPrevVictimSyncset(VictimSyncset& prev_victim_syncset) const
    {
        if (prev_victim_syncset_ptr_ == NULL)
        {
            return false;
        }
        else
        {
            prev_victim_syncset = *prev_victim_syncset_ptr_; // Deep copy
            return true;
        }
    }

    void VictimsyncMonitor::setPrevVictimSyncset(const VictimSyncset& prev_victim_syncset)
    {
        if (prev_victim_syncset_ptr_ == NULL)
        {
            prev_victim_syncset_ptr_ = new VictimSyncset(prev_victim_syncset); // Copy constructor
            assert(prev_victim_syncset_ptr_ != NULL);
        }
        else
        {
            // NOTE: is_enforce_complete of prev_victim_syncset will NOT be used in compression later
            *prev_victim_syncset_ptr_ = prev_victim_syncset; // Deep copy
        }
        return;
    }

    void VictimsyncMonitor::releasePrevVictimSyncset()
    {
        if (prev_victim_syncset_ptr_ != NULL)
        {
            delete prev_victim_syncset_ptr_;
            prev_victim_syncset_ptr_ = NULL;
        }
        return;
    }

    bool VictimsyncMonitor::needEnforcement() const
    {
        if (need_enforcement_)
        {
            assert(wait_for_complete_victim_syncset_ == true); // NOTE: current edge node MUST be waiting for a complete victim syncset from the specific neighbor if need enforcement
        }
        return need_enforcement_;
    }

    void VictimsyncMonitor::resetEnforcement()
    {
        need_enforcement_ = false;
        return;
    }

    // As receiver edge node

    bool VictimsyncMonitor::isFirstReceived() const
    {
        return is_first_received_;
    }

    void VictimsyncMonitor::clearFirstReceived(const uint32_t& synced_seqnum)
    {
        assert(synced_seqnum == 0); // NOTE: the first received victim syncset MUST have a seqnum of 0

        is_first_received_ = false;
        tracked_seqnum_ = synced_seqnum;
        return;
    }

    SeqNum VictimsyncMonitor::getTrackedSeqnum() const
    {
        return tracked_seqnum_;
    }

    SeqNum VictimsyncMonitor::getEnforcementSeqnum() const
    {
        return enforcement_seqnum_;
    }

    bool VictimsyncMonitor::isWaitForCompleteVictimSyncset() const
    {
        return wait_for_complete_victim_syncset_;
    }

    // Utils

    uint64_t VictimsyncMonitor::getSizeForCapacity() const
    {
        uint64_t size = 0;

        size += sizeof(SeqNum); // cur_seqnum_
        if (prev_victim_syncset_ptr_ != NULL)
        {
            size += prev_victim_syncset_ptr_->getSizeForCapacity();
        }
        size += sizeof(bool); // need_enforcement_

        size += sizeof(bool); // is_first_received_
        size += sizeof(SeqNum); // tracked_seqnum_
        for (std::vector<VictimSyncset>::const_iterator iter = cached_victim_syncsets_.begin(); iter != cached_victim_syncsets_.end(); iter++)
        {
            size += iter->getSizeForCapacity(); // cached victim syncset
        }
        size += sizeof(SeqNum); // enforcement_seqnum_
        size += sizeof(bool); // wait_for_complete_victim_syncset_

        return size;
    }
}