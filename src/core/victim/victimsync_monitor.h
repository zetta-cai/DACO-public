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

        // As sender edge node

        // As receiver edge node

        bool isFirstReceived() const;
        SeqNum getTrackedSeqnum() const;
        SeqNum getEnforcementSeqnum() const;
        bool isWaitForCompleteVictimSyncset() const;

        void clearEnforcementStatus();
        void setEnforcementStatus(const SeqNum& synced_seqnum);
    private:
        static const std::string kClassName;

        // NOTE: dedup-/delta-based victim syncset compression/recovery MUST follow strict seqnum order (unless the received victim syncset for recovery is complete)
        // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)

        // As sender edge node
        SeqNum cur_seqnum_; // The seqnum for current victim syncset towards a specific neighbor
        VictimSyncset* prev_victim_syncset_ptr_; // Prev issued victim syncset towards a specific neighbor
        bool need_enforcement_; // If the current edge node needs to notify the specific neighbor for the enforcement on complete victim syncset (avoid duplicate notification)

        // As receiver edge node
        bool is_first_received_; // If the victim syncset is the first one from a specific neighbor
        SeqNum tracked_seqnum_; // The seqnum for tracked victim syncset from a specific neighbor
        std::vector<VictimSyncset> cached_victim_syncsets_; // Cached in-advance victim syncsets from a specific neighbor
        SeqNum enforcement_seqnum_; // The max seqnum between cached_victim_syncsets_ and the victim syncset triggering the enforcement of complete victim syncset from a specific neighbor
        bool wait_for_complete_victim_syncset_; // If the current edge node is waiting for a complete victim syncset from a specific neighbor (avoid duplicate enforcement)
    };
}

#endif