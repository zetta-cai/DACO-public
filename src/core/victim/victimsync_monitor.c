#include "core/victim/victimsync_monitor.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(NULL), is_first_complete_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false), need_enforcement_(false)
    {
        cached_victim_syncsets_.clear();
    }

    VictimsyncMonitor::~VictimsyncMonitor()
    {
        releasePrevVictimSyncset_();
    }

    // As sender edge node

    void VictimsyncMonitor::enforceComplete()
    {
        // Release prev victim syncset, pre-generated complete victim syncset, and pre-generated compressed victim syncset for the receiver edge node such that the next message will piggyback a complete victim syncset
        releasePrevVictimSyncset_();

        return;
    }

    void VictimsyncMonitor::replacePrevVictimSyncset(VictimSyncset& current_victim_syncset, VictimSyncset& final_victim_syncset)
    {
        assert(current_victim_syncset.isComplete());

        // Get correct seqnum and need_enforcement_ flag
        const SeqNum final_seqnum = getCurSeqnum_();
        bool final_is_enforce_complete = isNeedEnforcement_();

        // Incr cur_seqnum_ by 1 and reset need_enforcement_ flag
        incrCurSeqnum_();
        resetNeedEnforcement_();

        // Set correct seqnum and need_enforcement_ flag for the latest victim syncset
        current_victim_syncset.setSeqnum(final_seqnum);
        current_victim_syncset.setEnforceComplete(final_is_enforce_complete);

        // Get final victim syncset with compression if necessary
        if (prev_victim_syncset_ptr_ != NULL)
        {
            assert(prev_victim_syncset_ptr_->isComplete());

            // Calculate compressed victim syncset by dedup/delta-compression based on current and prev complete victim syncset
            final_victim_syncset = VictimSyncset::compress(current_victim_syncset, *prev_victim_syncset_ptr_);
        }
        else
        {
            final_victim_syncset = current_victim_syncset;
        }

        // Replace prev-issued victim syncset
        releasePrevVictimSyncset_();
        prev_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);
        assert(prev_victim_syncset_ptr_ != NULL);
        
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

    bool VictimsyncMonitor::tryToClearEnforcementStatus_(const VictimSyncset& neighbor_complete_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt, VictimSyncset& new_neighbor_complete_victim_syncset)
    {
        assert(neighbor_complete_victim_syncset.isComplete());
        assert(neighbor_complete_victim_syncset.getSeqnum() == synced_seqnum);

        // Set tracked seqnum as synced seqnum
        tracked_seqnum_ = synced_seqnum;

        // Clear stale cached victim syncsets based on updated tracked seqnum
        clearStaleCachedVictimSyncsets_(peredge_monitored_victimsetcnt);

        // Clear continuous cached victim syncsets and update tracked sequm if any
        bool with_new_neighbor_complete_victim_syncset = clearContinuousCachedVictimSyncsets_(neighbor_complete_victim_syncset, peredge_monitored_victimsetcnt, new_neighbor_complete_victim_syncset);
        assert(tracked_seqnum_ >= synced_seqnum);

        if (wait_for_complete_victim_syncset_ && (tracked_seqnum_ > enforcement_seqnum_)) // Equivalent to receiving complete victim syncset after enforcing sender to clear prev victim syncset history for the current edge node
        {
            // Clear enforcement status
            need_enforcement_ = false;
            enforcement_seqnum_ = 0;
            wait_for_complete_victim_syncset_ = false;
        }

        return with_new_neighbor_complete_victim_syncset;
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

    // Dump/load victim sync monitor for synchronized victims of covered cache manager snapshot

    void VictimsyncMonitor::dumpVictimsyncMonitor(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump cur_seqnum_
        fs_ptr->write((const char*)&cur_seqnum_, sizeof(SeqNum));

        // NOTE: we do NOT dump prev_victim_syncset_ptr_, such that each cache node will enforce complete victim synchronization first after loading snapshot to continue stresstest phase

        // Dump is_first_complete_received_
        fs_ptr->write((const char*)&is_first_complete_received_, sizeof(bool));

        // Dump tracked_seqnum_
        fs_ptr->write((const char*)&tracked_seqnum_, sizeof(SeqNum));

        // Dump cached victim syncsets
        // (1) cached victim syncset cnt
        uint32_t cached_victim_syncset_cnt = cached_victim_syncsets_.size();
        fs_ptr->write((const char*)&cached_victim_syncset_cnt, sizeof(uint32_t));
        // (2) cached victim syncsets
        for (uint32_t i = 0; i < cached_victim_syncset_cnt; i++)
        {
            cached_victim_syncsets_[i].serialize(fs_ptr);
        }

        return;
    }

    void VictimsyncMonitor::loadVictimsyncMonitor(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load cur_seqnum_
        fs_ptr->read((char*)&cur_seqnum_, sizeof(SeqNum));

        // NOTE: we do NOT load prev_victim_syncset_ptr_, such that each cache node will enforce complete victim synchronization first after loading snapshot to continue stresstest phase
        prev_victim_syncset_ptr_ = NULL;

        // Load is_first_complete_received_
        fs_ptr->read((char*)&is_first_complete_received_, sizeof(bool));

        // Load tracked_seqnum_
        fs_ptr->read((char*)&tracked_seqnum_, sizeof(SeqNum));

        // Load cached victim syncsets
        // (1) cached victim syncset cnt
        uint32_t cached_victim_syncset_cnt = 0;
        fs_ptr->read((char*)&cached_victim_syncset_cnt, sizeof(uint32_t));
        // (2) cached victim syncsets
        cached_victim_syncsets_.clear();
        for (uint32_t i = 0; i < cached_victim_syncset_cnt; i++)
        {
            VictimSyncset tmp_victim_syncset;
            tmp_victim_syncset.deserialize(fs_ptr);
            cached_victim_syncsets_.push_back(tmp_victim_syncset);
        }

        return;
    }

    // As sender edge node

    void VictimsyncMonitor::releasePrevVictimSyncset_()
    {
        if (prev_victim_syncset_ptr_ != NULL)
        {
            delete prev_victim_syncset_ptr_;
            prev_victim_syncset_ptr_ = NULL;
        }
        return;
    }

    SeqNum VictimsyncMonitor::getCurSeqnum_()
    {
        // NOTE: dedup-/delta-based victim syncset compression MUST follow strict seqnum order
        if (prev_victim_syncset_ptr_ != NULL)
        {
            assert(cur_seqnum_ == Util::uint64Add(prev_victim_syncset_ptr_->getSeqnum(), 1));
        }

        return cur_seqnum_;
    }

    void VictimsyncMonitor::incrCurSeqnum_()
    {
        cur_seqnum_ = Util::uint64Add(cur_seqnum_, 1);
        return;
    }

    bool VictimsyncMonitor::isNeedEnforcement_()
    {
        bool tmp_need_enforcement = need_enforcement_;

        // (OBSOLETE due to NOT reset need_enforcement_ in pre-compression) Check if there exists any victim syncset w/ is_enforce_complete_ as true, yet has NOT been issued to the corresponding edge node
        // if (!tmp_need_enforcement)
        // {
        //     if (pregen_complete_victim_syncset_ptr_ != NULL)
        //     {
        //         tmp_need_enforcement = pregen_complete_victim_syncset_ptr_->isEnforceComplete();
        //     }
        // }

        return tmp_need_enforcement;
    }

    void VictimsyncMonitor::resetNeedEnforcement_()
    {
        need_enforcement_ = false;
        return;
    }

    // As receiver edge node

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

    bool VictimsyncMonitor::clearContinuousCachedVictimSyncsets_(const VictimSyncset& neighbor_complete_victim_syncset, const uint32_t& peredge_monitored_victimsetcnt, VictimSyncset& new_neighbor_complete_victim_syncset)
    {
        // NOTE: neighbor_complete_victim_syncset and new_neighbor_complete_victim_syncset are allowed to refer the same object, as we will NEVER use neighbor_complete_victim_syncset after setting new_neighbor_complete_victim_syncset in this function
        
        assert(neighbor_complete_victim_syncset.isComplete());
        assert(tracked_seqnum_ == neighbor_complete_victim_syncset.getSeqnum()); // NOTE: tracked_seqnum_ MUST already be updated as the current complete victim syncset before clearContinuousCachedVictimSyncsets_()
        assert(cached_victim_syncsets_.size() <= peredge_monitored_victimsetcnt);

        // Start from the current complete victim syncset
        bool with_new_neighbor_complete_victim_syncset = false;
        //new_neighbor_complete_victim_syncset = neighbor_complete_victim_syncset;

        while (true)
        {
            bool is_continuous_exist = false;

            // Try to find continuous cached victim syncset to update final victim syncset
            for (std::vector<VictimSyncset>::const_iterator iter = cached_victim_syncsets_.begin(); iter != cached_victim_syncsets_.end(); iter++)
            {
                assert(iter->isCompressed() == true); // NOTE: only compressed victim syncsets are cached in cached_victim_syncsets_

                const SeqNum tmp_cached_seqnum = iter->getSeqnum();

                assert(tmp_cached_seqnum > tracked_seqnum_); // NOTE: stale cached victim syncsets have been cleared by clearStaleCachedVictimSyncsets_()

                if (tmp_cached_seqnum == Util::uint64Add(tracked_seqnum_, 1)) // Continuous cached victim syncset
                {
                    // Recover based on the continuous cached victim syncset
                    if (!with_new_neighbor_complete_victim_syncset)
                    {
                        with_new_neighbor_complete_victim_syncset = true;
                        new_neighbor_complete_victim_syncset = VictimSyncset::recover(*iter, neighbor_complete_victim_syncset);
                    }
                    else
                    {
                        new_neighbor_complete_victim_syncset = VictimSyncset::recover(*iter, new_neighbor_complete_victim_syncset);
                    }
                    assert(new_neighbor_complete_victim_syncset.isComplete());
                    assert(new_neighbor_complete_victim_syncset.getSeqnum() == tmp_cached_seqnum);

                    // Clear the continuous cached victim syncset
                    cached_victim_syncsets_.erase(iter);
                    iter = cached_victim_syncsets_.end();

                    // Update tracked seqnum as the lastet complete victim syncset
                    tracked_seqnum_ = tmp_cached_seqnum;

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

        if (with_new_neighbor_complete_victim_syncset)
        {
            assert(new_neighbor_complete_victim_syncset.isComplete());
        }
        return with_new_neighbor_complete_victim_syncset;
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