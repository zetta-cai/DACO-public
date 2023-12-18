#include "core/victim/victimsync_monitor.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(NULL), pregen_complete_victim_syncset_ptr_(NULL), pregen_compressed_victim_syncset_ptr_(NULL), is_first_complete_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false), need_enforcement_(false)
    {
        cached_victim_syncsets_.clear();
    }
    #else
    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(NULL), is_first_complete_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false), need_enforcement_(false)
    {
        cached_victim_syncsets_.clear();
    }
    #endif

    VictimsyncMonitor::~VictimsyncMonitor()
    {
        releasePrevVictimSyncset_();
        #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
        releasePregenCompleteVictimSyncset_();
        releasePregenCompressedVictimSyncset_();
        #endif
    }

    // As sender edge node

    void VictimsyncMonitor::enforceComplete()
    {
        // Release prev victim syncset, pre-generated complete victim syncset, and pre-generated compressed victim syncset for the receiver edge node such that the next message will piggyback a complete victim syncset
        releasePrevVictimSyncset_();
        #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
        releasePregenCompleteVictimSyncset_();
        releasePregenCompressedVictimSyncset_();
        #endif

        return;
    }

    #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
    void VictimsyncMonitor::pregenVictimSyncset(const VictimSyncset& current_victim_syncset)
    {
        assert(current_victim_syncset.isComplete());

        if (prev_victim_syncset_ptr_ == NULL)
        {
            // Get correct seqnum and need_enforcement_ flag
            const SeqNum final_seqnum = getCurSeqnum_();
            bool final_is_enforce_complete = isNeedEnforcement_();

            // Update pre-generated complete victim syncset
            releasePregenCompleteVictimSyncset_();
            pregen_complete_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);
            pregen_complete_victim_syncset_ptr_->setSeqnum(final_seqnum);
            pregen_complete_victim_syncset_ptr_->setEnforceComplete(final_is_enforce_complete);

            // Release pre-generated compressed victim syncset
            releasePregenCompressedVictimSyncset_();
        }
        else
        {
            assert(prev_victim_syncset_ptr_->isComplete());

            // Get correct seqnum and need_enforcement_ flag
            const SeqNum final_seqnum = getCurSeqnum_();
            bool final_is_enforce_complete = isNeedEnforcement_();

            // Update pre-generated complete victim syncset
            releasePregenCompleteVictimSyncset_();
            pregen_complete_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);
            pregen_complete_victim_syncset_ptr_->setSeqnum(final_seqnum);
            pregen_complete_victim_syncset_ptr_->setEnforceComplete(final_is_enforce_complete);

            // Calculate compressed victim syncset by dedup/delta-compression based on current and prev complete victim syncset
            VictimSyncset compressed_victim_syncset = VictimSyncset::compress(*pregen_complete_victim_syncset_ptr_, *prev_victim_syncset_ptr_);

            // Update pre-generated compressed victim syncset if necessary
            releasePregenCompressedVictimSyncset_();
            if (compressed_victim_syncset.isCompressed())
            {
                pregen_compressed_victim_syncset_ptr_ = new VictimSyncset(compressed_victim_syncset);

                // NOTE: must with the same seqnum and is_enforce_complete flag as pre-generated complete victim syncset
                assert(pregen_compressed_victim_syncset_ptr_->getSeqnum() == final_seqnum);
                assert(pregen_compressed_victim_syncset_ptr_->isEnforceComplete() == final_is_enforce_complete);
            }
        }

        return;
    }

    bool VictimsyncMonitor::getPregenVictimSyncset(VictimSyncset& final_victim_syncset)
    {
        bool need_latest_victim_syncset = false;

        if (pregen_complete_victim_syncset_ptr_ == NULL) // No pre-compression before
        {
            assert(pregen_compressed_victim_syncset_ptr_ == NULL);

            if (prev_victim_syncset_ptr_ == NULL) // First-issued or enforce-complete victim syncset
            {
                // NOTE: need_enforcement_ could be true as receiver if enforce-complete as sender

                // VictimTracker needs to generate the latest local victim syncset and replace prev-issued one later
                need_latest_victim_syncset = true;
            }
            else // No pre-compression before
            {
                // TODO: we can also set need_latest_victim_syncset = true to synchronize latest victim info, yet with extra time cost to get latest victim info

                // NOTE: here we just simply treat current victim syncset is the same as prev-issued one to avoid time cost of getting latest victim info

                assert(prev_victim_syncset_ptr_->isComplete());

                // Get correct seqnum and need_enforcement_ flag
                const SeqNum final_seqnum = getCurSeqnum_();
                const bool final_is_enforce_complete = isNeedEnforcement_();
                assert(final_is_enforce_complete == false); // need_enforcement_ MUST be false (as no neighbor victim syncset is received since the last issued message)

                // Incr cur_seqnum_ by 1 (NO need to reset need_enforcement_ due to already false)
                incrCurSeqnum_();

                // Generate a full-compressed victim syncset w/ correct seqnum
                final_victim_syncset = VictimSyncset::getFullCompressedVictimSyncset(final_seqnum, final_is_enforce_complete);

                // NOTE: NO need to replace the entire prev_victim_syncset_ptr_, yet still need to update the seqnum (is_enforce_complete of prev-issued victim syncset is NOT used)
                prev_victim_syncset_ptr_->setSeqnum(final_seqnum);
            }
        }
        else // With pre-compression before
        {
            assert(pregen_complete_victim_syncset_ptr_->isComplete());

            if (pregen_compressed_victim_syncset_ptr_ == NULL) // Pre-generate complete victim syncset
            {
                final_victim_syncset = *pregen_complete_victim_syncset_ptr_;
            }
            else // Pre-generate compressed victim syncset
            {
                assert(prev_victim_syncset_ptr_ != NULL);
                assert(prev_victim_syncset_ptr_->isComplete());
                assert(pregen_compressed_victim_syncset_ptr_->isCompressed());
                assert(pregen_complete_victim_syncset_ptr_->isEnforceComplete() == pregen_compressed_victim_syncset_ptr_->isEnforceComplete());

                final_victim_syncset = *pregen_compressed_victim_syncset_ptr_;
            }

            // Incr cur_seqnum_ by 1 and reset need_enforcement_ flag
            assert(final_victim_syncset.getSeqnum() == cur_seqnum_);
            assert(final_victim_syncset.isEnforceComplete() == need_enforcement_);
            incrCurSeqnum_();
            resetNeedEnforcement_();

            // Replace prev-issued complete victim syncset with pre-generated complete victim syncset for the next pre-compression
            delete prev_victim_syncset_ptr_;
            prev_victim_syncset_ptr_ = pregen_complete_victim_syncset_ptr_;
            pregen_complete_victim_syncset_ptr_ = NULL;
            
            // Release pre-generated compressed victim syncset if any
            releasePregenCompressedVictimSyncset_();
        }

        return need_latest_victim_syncset;
    }

    void VictimsyncMonitor::replacePrevVictimSyncset(const VictimSyncset& current_victim_syncset, VictimSyncset& final_victim_syncset)
    {
        assert(current_victim_syncset.isComplete());
        assert(pregen_compressed_victim_syncset_ptr_ == NULL);

        //bool is_prev_victim_syncset_exist = false;

        if (prev_victim_syncset_ptr_ != NULL)
        {
            assert(prev_victim_syncset_ptr_->isComplete());

            //is_prev_victim_syncset_exist = true;
            //prev_victim_syncset = *prev_victim_syncset_ptr_; // Deep copy
        }

        // Get correct seqnum and need_enforcement_ flag
        const SeqNum final_seqnum = getCurSeqnum_();
        bool final_is_enforce_complete = isNeedEnforcement_();

        // Incr cur_seqnum_ by 1 and reset need_enforcement_ flag
        incrCurSeqnum_();
        resetNeedEnforcement_();

        // Replace prev-issued victim syncset
        releasePrevVictimSyncset_();
        prev_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);
        prev_victim_syncset_ptr_->setSeqnum(final_seqnum);
        prev_victim_syncset_ptr_->setEnforceComplete(final_is_enforce_complete);

        final_victim_syncset = *prev_victim_syncset_ptr_; // Deep copy

        return;
        //return is_prev_victim_syncset_exist;
    }

    #else

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

    #endif

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
        #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
        if (pregen_complete_victim_syncset_ptr_ != NULL)
        {
            size += pregen_complete_victim_syncset_ptr_->getSizeForCapacity();
        }
        if (pregen_compressed_victim_syncset_ptr_ != NULL)
        {
            size += pregen_compressed_victim_syncset_ptr_->getSizeForCapacity();
        }
        #endif
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

    #ifdef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
    void VictimsyncMonitor::releasePregenCompleteVictimSyncset_()
    {
        if (pregen_complete_victim_syncset_ptr_ != NULL)
        {
            delete pregen_complete_victim_syncset_ptr_;
            pregen_complete_victim_syncset_ptr_ = NULL;
        }
        return;
    }

    void VictimsyncMonitor::releasePregenCompressedVictimSyncset_()
    {
        if (pregen_compressed_victim_syncset_ptr_ != NULL)
        {
            delete pregen_compressed_victim_syncset_ptr_;
            pregen_compressed_victim_syncset_ptr_ = NULL;
        }
        return;
    }
    #endif

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