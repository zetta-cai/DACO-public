#include "core/victim/victimsync_monitor.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(nullptr), need_enforcement_(false), is_first_complete_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false)
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

    bool VictimsyncMonitor::isFirstCompleteReceived() const
    {
        return is_first_complete_received_;
    }

    void VictimsyncMonitor::clearFirstCompleteReceived()
    {
        assert(tracked_seqnum_ == 0); // NOTE: before the first complete victim syncset is received, tracked seqnum MUST be 0 after initialization

        is_first_complete_received_ = false;
        return;
    }

    SeqNum VictimsyncMonitor::getTrackedSeqnum() const
    {
        return tracked_seqnum_;
    }

    VictimSyncset VictimsyncMonitor::tryToClearEnforcementStatus_(const VictimSyncset& neighbor_complete_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt)
    {
        assert(neighbor_complete_victim_syncset.isComplete());
        assert(neighbor_complete_victim_syncset.getSeqnum() == synced_seqnum);

        // Set tracked seqnum as synced seqnum
        tracked_seqnum_ = synced_seqnum;

        // Clear stale cached victim syncsets based on updated tracked seqnum
        clearStaleCachedVictimSyncsets_(peredge_monitored_victimsetcnt);

        // Clear continuous cached victim syncsets and update tracked sequm if any
        VictimSyncset final_victim_syncset = clearContinuousCachedVictimSyncsets_(neighbor_complete_victim_syncset, peredge_monitored_victimsetcnt);
        assert(tracked_seqnum_ >= synced_seqnum);

        if (wait_for_complete_victim_syncset_ && (tracked_seqnum_ > enforcement_seqnum_)) // Equivalent to receiving complete victim syncset after enforcing sender to clear prev victim syncset history for the current edge node
        {
            // Clear enforcement status
            need_enforcement_ = false;
            enforcement_seqnum_ = 0;
            wait_for_complete_victim_syncset_ = false;
        }

        return final_victim_syncset;
    }

    void VictimsyncMonitor::tryToEnableEnforcementStatus_(const VictimSyncset& neighbor_compressed_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt)
    {
        assert(neighbor_compressed_victim_syncset.isCompressed());
        assert(neighbor_compressed_victim_syncset.getSeqnum() == synced_seqnum);

        if (cached_victim_syncsets_.size() < peredge_monitored_victimsetcnt) // NOT full
        {
            cached_victim_syncsets_.push_back(neighbor_compressed_victim_syncset);
        }
        else if (wait_for_complete_victim_syncset_ == false) // Full and NOT trigger complete enforcement before
        {
            // Set enforcement_seqnum_ = (max seqnum between cached victim syncsets and synced seqnum)
            const SeqNum tmp_max_seqnum = getMaxSeqnumFromCachedVictimSyncsets_(peredge_monitored_victimsetcnt);
            enforcement_seqnum_ = synced_seqnum >= tmp_max_seqnum ? synced_seqnum : tmp_max_seqnum;

            wait_for_complete_victim_syncset_ = true;

            assert(need_enforcement_ == false);
            need_enforcement_ = true;
        }

        return;
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

    void VictimsyncMonitor::clearStaleCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt)
    {
        assert(cached_victim_syncsets_.size() <= peredge_monitored_victimsetcnt);

        for (std::vector<VictimSyncset>::iterator iter = cached_victim_syncsets_.begin(); iter != cached_victim_syncsets_.end();)
        {
            if (iter->getSeqnum() <= tracked_seqnum_)
            {
                iter = cached_victim_syncsets_.erase(iter);
            }
            else
            {
                iter++;
            }
        }

        return;
    }

    VictimSyncset VictimsyncMonitor::clearContinuousCachedVictimSyncsets_(const VictimSyncset& neighbor_complete_victim_syncset, const uint32_t& peredge_monitored_victimsetcnt)
    {
        assert(neighbor_complete_victim_syncset.isComplete());
        assert(tracked_seqnum_ == neighbor_complete_victim_syncset.getSeqnum()); // NOTE: tracked_seqnum_ MUST already be updated as the current complete victim syncset before clearContinuousCachedVictimSyncsets_()
        assert(cached_victim_syncsets_.size() <= peredge_monitored_victimsetcnt);

        VictimSyncset final_victim_syncset = neighbor_complete_victim_syncset; // Start from the current complete victim syncset
        while (true)
        {
            bool is_continuous_exist = false;

            // Try to find continuous cached victim syncset to update final victim syncset
            SeqNum tmp_final_seqnum = final_victim_syncset.getSeqnum();
            for (std::vector<VictimSyncset>::const_iterator iter = cached_victim_syncsets_.begin(); iter != cached_victim_syncsets_.end(); iter++)
            {
                assert(iter->isCompressed() == true); // NOTE: only compressed victim syncsets are cached in cached_victim_syncsets_

                const SeqNum tmp_cached_seqnum = iter->getSeqnum();

                assert(tmp_cached_seqnum > tmp_final_seqnum); // NOTE: stale cached victim syncsets have been cleared by clearStaleCachedVictimSyncsets_()

                if (tmp_cached_seqnum == Util::uint64Add(tmp_final_seqnum, 1)) // Continuous cached victim syncset
                {
                    // Recover based on the continuous cached victim syncset
                    final_victim_syncset = VictimSyncset::recover(*iter, final_victim_syncset);
                    assert(final_victim_syncset.isComplete());
                    assert(final_victim_syncset.getSeqnum() == tmp_cached_seqnum);

                    // Clear the continuous cached victim syncset
                    cached_victim_syncsets_.erase(iter);
                    iter = cached_victim_syncsets_.end();

                    // Update tracked seqnum as the lastet complete victim syncset
                    tracked_seqnum_ = final_victim_syncset.getSeqnum();

                    is_continuous_exist = true;
                    break;
                }
            }

            // No continuous cached victim syncset for final victim syncset
            if (!is_continuous_exist)
            {
                break;
            }
        }

        assert(final_victim_syncset.isComplete());
        return final_victim_syncset;
    }

    SeqNum VictimsyncMonitor::getMaxSeqnumFromCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt) const
    {
        assert(cached_victim_syncsets_.size() <= peredge_monitored_victimsetcnt);

        SeqNum max_seqnum = 0;
        for (std::vector<VictimSyncset>::const_iterator iter = cached_victim_syncsets_.begin(); iter != cached_victim_syncsets_.end(); iter++)
        {
            const SeqNum tmp_seqnum = iter->getSeqnum();
            if (iter == cached_victim_syncsets_.begin())
            {
                max_seqnum = tmp_seqnum;
            }
            else if (max_seqnum < tmp_seqnum)
            {
                max_seqnum = tmp_seqnum;
            }
        }

        return max_seqnum;
    }
}