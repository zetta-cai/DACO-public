/*
 * VictimsyncMonitor: sequence-based victim synchronization monitor for a specific neighbor to detect victim synchronization anomalies caused by packet loss/reordering, cache in-advance victim syncsets, and enforces the sender edge node to issue complete victim syncsets without compression if necessary (with limited extra communication overhead).
 * 
 * By Siyuan Sheng (2023.10.28).
 */

#ifndef VICTIMSYNC_MONITOR_H
#define VICTIMSYNC_MONITOR_H

#include <string>

#include "common/covered_common_header.h"
#include "core/victim/victim_syncset.h"

namespace covered
{
    class VictimsyncMonitor
    {
    public:
        VictimsyncMonitor();
        ~VictimsyncMonitor();

        bool isValid() const;
        void validate();

        // As sender edge node

        SeqNum getCurSeqnum() const;
        void incrCurSeqnum();

        void setPrevVictimSyncset(const VictimSyncset& prev_victim_syncset);
        bool getPregenCompleteVictimSyncset(VictimSyncset& pregen_complete_victim_syncset) const; // Returhn if pre-generated complete victim syncset exists
        void setPregenCompleteVictimSyncset(const VictimSyncset& pregen_complete_victim_syncset);
        bool getPregenCompressedVictimSyncset(VictimSyncset& pregen_compressed_victim_syncset) const; // Return if pre-generated compressed victim syncset exists
        void setPregenCompressedVictimSyncset(const VictimSyncset& pregen_compressed_victim_syncset);

        void enforceComplete(); // Enforce a complete victim syncset for the next message to the receiver
        void pregenVictimSyncset(const VictimSyncset& current_victim_syncset); // Pre-generate complete/compressed victim syncset for the next message to the receiver
        bool getPregenVictimSyncset(VictimSyncset& final_victim_syncset); // Return if need the latest victim syncset

        // As receiver edge node

        bool isFirstCompleteReceived() const;
        void clearFirstCompleteReceived();

        SeqNum getTrackedSeqnum() const;

        VictimSyncset tryToClearEnforcementStatus_(const VictimSyncset& neighbor_complete_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt); // If no continous compressed victim syncsets in cached_victim_syncsets_, returned victim syncset will be the same as neighbor_complete_victim_syncset
        void tryToEnableEnforcementStatus_(const VictimSyncset& neighbor_compressed_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt);

        // NOTE: need enforcement flag will be embedded into the next message sent to the sender
        bool needEnforcement() const;
        void resetEnforcement();

        // Utils

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        void releasePrevVictimSyncset_();
        void releasePregenCompleteVictimSyncset_();
        void releasePregenCompressedVictimSyncset_();

        void clearStaleCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt);
        VictimSyncset clearContinuousCachedVictimSyncsets_(const VictimSyncset& neighbor_complete_victim_syncset, const uint32_t& peredge_monitored_victimsetcnt); // If no continous compressed victim syncsets in cached_victim_syncsets_, returned victim syncset will be the same as neighbor_complete_victim_syncset (this could update tracked seqnum)
        SeqNum getMaxSeqnumFromCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt) const;

        bool is_valid_;
        void checkValidity_() const;

        // NOTE: dedup-/delta-based victim syncset compression/recovery MUST follow strict seqnum order (unless the received victim syncset for recovery is complete)
        // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)

        // As sender edge node
        SeqNum cur_seqnum_; // The seqnum for current victim syncset towards a specific neighbor
        VictimSyncset* prev_victim_syncset_ptr_; // Prev issued victim syncset towards a specific neighbor for dedup-/delta-based compression
        VictimSyncset* pregen_complete_victim_syncset_ptr_; // Pre-generated complete victim syncset for background pre-compression
        VictimSyncset* pregen_compressed_victim_syncset_ptr_; // Pre-generated compressed victim syncset for background pre-compression
        bool same_as_prev_; // Is the current victim syncset is the same as the last issued one

        // As receiver edge node
        bool is_first_complete_received_; // If the victim syncset is the first complete one from a specific neighbor
        SeqNum tracked_seqnum_; // The seqnum for tracked victim syncset from a specific neighbor
        std::vector<VictimSyncset> cached_victim_syncsets_; // Cached in-advance compressed victim syncsets from a specific neighbor
        SeqNum enforcement_seqnum_; // The max seqnum between cached_victim_syncsets_ and the victim syncset triggering the enforcement of complete victim syncset from a specific neighbor
        bool wait_for_complete_victim_syncset_; // If the current edge node is waiting for a complete victim syncset from a specific neighbor (avoid duplicate enforcement)
        bool need_enforcement_; // If the current edge node needs to notify the specific neighbor for the enforcement on complete victim syncset (avoid duplicate notification)
    };
}

#endif