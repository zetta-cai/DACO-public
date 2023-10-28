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

        SeqNum getCurSeqnum() const;
        void incrCurSeqnum();

        bool getPrevVictimSyncset(VictimSyncset& prev_victim_syncset) const; // Return if prev victim syncset exists
        void setPrevVictimSyncset(const VictimSyncset& prev_victim_syncset);
        void releasePrevVictimSyncset();

        bool needEnforcement() const;
        void resetEnforcement();

        // As receiver edge node

        bool isFirstReceived() const;
        void clearFirstReceived(const uint32_t& synced_seqnum);

        SeqNum getTrackedSeqnum() const;

        void tryToClearEnforcementStatus_(const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt);
        void tryToEnableEnforcementStatus_(const uint32_t& peredge_monitored_victimsetcnt, const VictimSyncset& neighbor_compressed_victim_syncset, const SeqNum& synced_seqnum);

        // Utils

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        void clearStaleCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt);
        SeqNum getMaxSeqnumFromCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt) const;

        // NOTE: dedup-/delta-based victim syncset compression/recovery MUST follow strict seqnum order (unless the received victim syncset for recovery is complete)
        // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)

        // As sender edge node
        SeqNum cur_seqnum_; // The seqnum for current victim syncset towards a specific neighbor
        VictimSyncset* prev_victim_syncset_ptr_; // Prev issued victim syncset towards a specific neighbor for dedup-/delta-based compression
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