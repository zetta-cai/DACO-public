#include "core/victim/victimsync_monitor.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    VictimsyncMonitor::VictimsyncMonitor() : is_valid_(false), cur_seqnum_(0), prev_victim_syncset_ptr_(NULL), pregen_complete_victim_syncset_ptr_(NULL), pregen_compressed_victim_syncset_ptr_(NULL), same_as_prev_(false), is_first_complete_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false), need_enforcement_(false)
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

        if (pregen_complete_victim_syncset_ptr_ != NULL)
        {
            delete pregen_complete_victim_syncset_ptr_;
            pregen_complete_victim_syncset_ptr_ = NULL;
        }

        if (pregen_compressed_victim_syncset_ptr_ != NULL)
        {
            delete pregen_compressed_victim_syncset_ptr_;
            pregen_compressed_victim_syncset_ptr_ = NULL;
        }
    }

    bool VictimsyncMonitor::isValid() const
    {
        return is_valid_;
    }

    void VictimsyncMonitor::validate()
    {
        assert(!is_valid_);
        is_valid_ = true;
        return;
    }

    // As sender edge node
    
    SeqNum VictimsyncMonitor::getCurSeqnum() const
    {
        checkValidity_();

        return cur_seqnum_;
    }

    void VictimsyncMonitor::incrCurSeqnum()
    {
        checkValidity_();

        cur_seqnum_ = Util::uint64Add(cur_seqnum_, 1);
        return;
    }

    void VictimsyncMonitor::setPrevVictimSyncset(const VictimSyncset& prev_victim_syncset)
    {
        checkValidity_();

        if (prev_victim_syncset_ptr_ == NULL)
        {
            prev_victim_syncset_ptr_ = new VictimSyncset(prev_victim_syncset); // Copy constructor
            assert(prev_victim_syncset_ptr_ != NULL);
        }
        else
        {
            // NOTE: is_enforce_complete of prev_victim_syncset will NOT be used in compression (keep is_enforce_complete of the latest victim syncset) and hence NOT embedded into message later
            *prev_victim_syncset_ptr_ = prev_victim_syncset; // Deep copy
        }
        return;
    }

    bool VictimsyncMonitor::getPregenCompleteVictimSyncset(VictimSyncset& pregen_complete_victim_syncset) const
    {
        checkValidity_();

        if (pregen_complete_victim_syncset_ptr_ == NULL)
        {
            return false;
        }
        else
        {
            pregen_complete_victim_syncset = *pregen_complete_victim_syncset_ptr_; // Deep copy
            return true;
        }
    }

    void VictimsyncMonitor::setPregenCompleteVictimSyncset(const VictimSyncset& pregen_complete_victim_syncset)
    {
        checkValidity_();

        if (pregen_complete_victim_syncset_ptr_ == NULL)
        {
            pregen_complete_victim_syncset_ptr_ = new VictimSyncset(pregen_complete_victim_syncset); // Copy constructor
            assert(pregen_complete_victim_syncset_ptr_ != NULL);
        }
        else
        {
            // NOTE: is_enforce_complete of pregen_complete_victim_syncset (i.e., the latest victim syncset when performing pre-compression) will be embedded into message later
            // NOTE: both pregen_complete_victim_syncset and pregen_compressed_vitim_syncset MUST have the same is_enforce_complete flag
            *pregen_complete_victim_syncset_ptr_ = pregen_complete_victim_syncset; // Deep copy
        }
        return;
    }

    bool VictimsyncMonitor::getPregenCompressedVictimSyncset(VictimSyncset& pregen_compressed_victim_syncset) const
    {
        checkValidity_();

        if (pregen_compressed_victim_syncset_ptr_ == NULL)
        {
            return false;
        }
        else
        {
            pregen_compressed_victim_syncset = *pregen_compressed_victim_syncset_ptr_; // Deep copy
            return true;
        }
    }

    void VictimsyncMonitor::setPregenCompressedVictimSyncset(const VictimSyncset& pregen_compressed_victim_syncset)
    {
        checkValidity_();

        if (pregen_compressed_victim_syncset_ptr_ == NULL)
        {
            pregen_compressed_victim_syncset_ptr_ = new VictimSyncset(pregen_compressed_victim_syncset); // Copy constructor
            assert(pregen_compressed_victim_syncset_ptr_ != NULL);
        }
        else
        {
            // NOTE: is_enforce_complete of pregen_compressed_victim_syncset (i.e., pre-compressed based on the latest victim syncset at that time) will NOT be embedded into message later
            // NOTE: both pregen_complete_victim_syncset and pregen_compressed_vitim_syncset MUST have the same is_enforce_complete flag
            *pregen_compressed_victim_syncset_ptr_ = pregen_compressed_victim_syncset; // Deep copy
        }
        return;
    }

    void VictimsyncMonitor::enforceComplete()
    {
        checkValidity_();

        // Release prev victim syncset, pre-generated complete victim syncset, and pre-generated compressed victim syncset for the receiver edge node such that the next message will piggyback a complete victim syncset
        releasePrevVictimSyncset_();
        releasePregenCompleteVictimSyncset_();
        releasePregenCompressedVictimSyncset_();

        return;
    }

    void VictimsyncMonitor::pregenVictimSyncset(const VictimSyncset& current_victim_syncset)
    {
        assert(current_victim_syncset.isComplete());

        if (prev_victim_syncset_ptr_ == NULL)
        {
            releasePregenCompleteVictimSyncset_();
            releasePregenCompressedVictimSyncset_();

            pregen_complete_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);
        }
        else
        {
            assert(prev_victim_syncset_ptr_->isComplete());

            // NOTE: dedup-/delta-based victim syncset compression MUST follow strict seqnum order
            if (pregen_complete_victim_syncset_ptr_ == NULL)
            {
                assert(current_victim_syncset.getSeqnum() == Util::uint64Add(prev_victim_syncset_ptr_->getSeqnum(), 1));
            }
            else
            {
                assert(current_victim_syncset.getSeqnum() == Util::uint64Add(pregen_complete_victim_syncset_ptr_->getSeqnum(), 1));
            }

            // Update pre-generated complete victim syncset
            releasePregenCompleteVictimSyncset_();
            pregen_complete_victim_syncset_ptr_ = new VictimSyncset(current_victim_syncset);

            // Calculate compressed victim syncset by dedup/delta-compression based on current and prev complete victim syncset
            VictimSyncset compressed_victim_syncset = VictimSyncset::compress(current_victim_syncset, *prev_victim_syncset_ptr_);

            // Update pre-generated compressed victim syncset if necessary
            releasePregenCompressedVictimSyncset_();
            if (compressed_victim_syncset.isCompressed())
            {
                pregen_compressed_victim_syncset_ptr_ = new VictimSyncset(compressed_victim_syncset);
            }
        }

        same_as_prev_ = false;
        return;
    }

    bool VictimsyncMonitor::getPregenVictimSyncset(VictimSyncset& final_victim_syncset)
    {
        bool need_latest_victim_syncset = false;

        if (same_as_prev_) // Current victim syncset is the same as the last issued one
        {
            assert(prev_victim_syncset_ptr_ != NULL);
            assert(pregen_complete_victim_syncset_ptr_ == NULL);
            assert(pregen_compressed_victim_syncset_ptr_ == NULL);

            // NOTE: must follow strict seqnum order
            assert(cur_seqnum_ == Util::uint64Add(prev_victim_syncset_ptr_->getSeqnum(), 1));

            // NOTE: need enforcement flag MUST be false, as no neighbor victim syncset is received since the last issued message
            assert(!need_enforcement_);

            // Generate a full-compressed victim syncset w/ correct seqnum
            const SeqNum final_seqnum = cur_seqnum_;
            incrCurSeqnum(); // Increase seqnum by 1
            const bool final_is_enforce_complete = false; // need_enforcement_ MUST be false and NO unissued victim syncset
            final_victim_syncset = VictimSyncset::getFullCompressedVictimSyncset(final_seqnum, final_is_enforce_complete);

            // NOTE: NO need to replace prev_victim_syncset_ptr_ and set same_as_prev_ due to same_as_prev_
        }
        else // Different from the last issued victim syncset
        {
            // TODO: END HERE
        }

        return need_latest_victim_syncset;
    }

    // As receiver edge node

    bool VictimsyncMonitor::isFirstCompleteReceived() const
    {
        checkValidity_();

        return is_first_complete_received_;
    }

    void VictimsyncMonitor::clearFirstCompleteReceived()
    {
        checkValidity_();

        assert(tracked_seqnum_ == 0); // NOTE: before the first complete victim syncset is received, tracked seqnum MUST be 0 after initialization

        is_first_complete_received_ = false;
        return;
    }

    SeqNum VictimsyncMonitor::getTrackedSeqnum() const
    {
        checkValidity_();

        return tracked_seqnum_;
    }

    VictimSyncset VictimsyncMonitor::tryToClearEnforcementStatus_(const VictimSyncset& neighbor_complete_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt)
    {
        checkValidity_();

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
        checkValidity_();

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

    bool VictimsyncMonitor::needEnforcement() const
    {
        checkValidity_();

        if (need_enforcement_)
        {
            assert(wait_for_complete_victim_syncset_ == true); // NOTE: current edge node MUST be waiting for a complete victim syncset from the specific neighbor if need enforcement
        }
        return need_enforcement_;
    }

    void VictimsyncMonitor::resetEnforcement()
    {
        checkValidity_();

        need_enforcement_ = false;
        return;
    }

    // Utils

    uint64_t VictimsyncMonitor::getSizeForCapacity() const
    {
        checkValidity_();

        uint64_t size = 0;

        size += sizeof(SeqNum); // cur_seqnum_
        if (prev_victim_syncset_ptr_ != NULL)
        {
            size += prev_victim_syncset_ptr_->getSizeForCapacity();
        }
        if (pregen_complete_victim_syncset_ptr_ != NULL)
        {
            size += pregen_complete_victim_syncset_ptr_->getSizeForCapacity();
        }
        if (pregen_compressed_victim_syncset_ptr_ != NULL)
        {
            size += pregen_compressed_victim_syncset_ptr_->getSizeForCapacity();
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

    void VictimsyncMonitor::releasePrevVictimSyncset_()
    {
        checkValidity_();

        if (prev_victim_syncset_ptr_ != NULL)
        {
            delete prev_victim_syncset_ptr_;
            prev_victim_syncset_ptr_ = NULL;
        }
        return;
    }

    void VictimsyncMonitor::releasePregenCompleteVictimSyncset_()
    {
        checkValidity_();

        if (pregen_complete_victim_syncset_ptr_ != NULL)
        {
            delete pregen_complete_victim_syncset_ptr_;
            pregen_complete_victim_syncset_ptr_ = NULL;
        }
        return;
    }

    void VictimsyncMonitor::releasePregenCompressedVictimSyncset_()
    {
        checkValidity_();

        if (pregen_compressed_victim_syncset_ptr_ != NULL)
        {
            delete pregen_compressed_victim_syncset_ptr_;
            pregen_compressed_victim_syncset_ptr_ = NULL;
        }
        return;
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

    void VictimsyncMonitor::checkValidity_() const
    {
        assert(is_valid_);
        return;
    }
}