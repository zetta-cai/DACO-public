/*
 * VictimsyncMonitor: sequence-based victim synchronization monitor for a specific neighbor to detect victim synchronization anomalies caused by packet loss/reordering, cache in-advance victim syncsets, and enforces the sender edge node to issue complete victim syncsets without compression if necessary (with limited extra communication overhead).
 * 
 * By Siyuan Sheng (2023.10.28).
 */

#ifndef VICTIMSYNC_MONITOR_H
#define VICTIMSYNC_MONITOR_H

#include <fstream>
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

        void enforceComplete(); // Enforce a complete victim syncset for the next message to the receiver
        void replacePrevVictimSyncset(VictimSyncset& current_victim_syncset, VictimSyncset& final_victim_syncset); // Try to get a compressed victim syncset based on the latest one and prev-issued one w/ correct seqnum and is_enforce_complete flag as final victim syncset, or directly used the latest one as final otherwise; then replace prev-issued victim syncset with the latest one

        // As receiver edge node

        bool isFirstCompleteReceived() const;
        void clearFirstCompleteReceived();

        SeqNum getTrackedSeqnum() const;

        bool tryToClearEnforcementStatus_(const VictimSyncset& neighbor_complete_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt, VictimSyncset& new_neighbor_complete_victim_syncset); // If no continous compressed victim syncsets in cached_victim_syncsets_, return false (i.e., new_neighbor_complete_victim_syncset is NOT set); this function could update tracked seqnum
        void tryToEnableEnforcementStatus_(const VictimSyncset& neighbor_compressed_victim_syncset, const SeqNum& synced_seqnum, const uint32_t& peredge_monitored_victimsetcnt);

        // Utils

        uint64_t getSizeForCapacity() const;

        // Dump/load victim sync monitor for synchronized victims of covered cache manager snapshot
        void dumpVictimsyncMonitor(std::fstream* fs_ptr) const;
        void loadVictimsyncMonitor(std::fstream* fs_ptr);
    private:
        static const std::string kClassName;

        // As sender edge node

        void releasePrevVictimSyncset_();

        // NOTE: we separate get&incr and get&reset as two steps, as getting correct seqnum or enforcement flag and issuing corresponding victim syncset happen asynchronously under background pre-compression
        SeqNum getCurSeqnum_();
        void incrCurSeqnum_();
        bool isNeedEnforcement_();
        void resetNeedEnforcement_();

        // As receiver edge node

        void clearStaleCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt);
        bool clearContinuousCachedVictimSyncsets_(const VictimSyncset& neighbor_complete_victim_syncset, const uint32_t& peredge_monitored_victimsetcnt, VictimSyncset& new_neighbor_complete_victim_syncset); // If no continous compressed victim syncsets in cached_victim_syncsets_, return false (i.e., new_neighbor_complete_victim_syncset is NOT set); this function could update tracked seqnum
        SeqNum getMaxSeqnumFromCachedVictimSyncsets_(const uint32_t& peredge_monitored_victimsetcnt) const;

        // NOTE: dedup-/delta-based victim syncset compression/recovery MUST follow strict seqnum order (unless the received victim syncset for recovery is complete)
        // NOTE: we assert that seqnum should NOT overflow if using uint64_t (TODO: fix it by integer wrapping in the future if necessary)

        // NOTE: both pregen_complete_victim_syncset and pregen_compressed_victim_syncset MUST have the same is_enforce_complete flag
        // ----> is_enforce_complete of prev_victim_syncset will NOT be used in compression (keep is_enforce_complete of the latest victim syncset) and hence NOT embedded into message later
        // ----> is_enforce_complete of pregen_complete_victim_syncset (i.e., the latest victim syncset when performing pre-compression) will be embedded into message later
        // ----> is_enforce_complete of pregen_compressed_victim_syncset (i.e., pre-compressed based on the latest victim syncset at that time) will NOT be embedded into message later

        // As sender edge node
        SeqNum cur_seqnum_; // The seqnum for current victim syncset towards a specific neighbor (cur_seqnum_ will be initialized as 0 such that the first victim syncset issued to a specific edge node will have a seqnum of 0)
        VictimSyncset* prev_victim_syncset_ptr_; // Prev issued victim syncset towards a specific neighbor for dedup-/delta-based compression

        // As receiver edge node
        bool is_first_complete_received_; // If the victim syncset is the first complete one from a specific neighbor (is_first_complete_received_ will be initialized as true such that the first received complete victim syncset can directly update victim tracker)
        SeqNum tracked_seqnum_; // The seqnum for tracked victim syncset from a specific neighbor
        std::vector<VictimSyncset> cached_victim_syncsets_; // Cached in-advance compressed victim syncsets from a specific neighbor
        SeqNum enforcement_seqnum_; // The max seqnum between cached_victim_syncsets_ and the victim syncset triggering the enforcement of complete victim syncset from a specific neighbor
        bool wait_for_complete_victim_syncset_; // If the current edge node is waiting for a complete victim syncset from a specific neighbor (avoid duplicate enforcement)
        bool need_enforcement_; // If the current edge node needs to notify the specific neighbor for the enforcement on complete victim syncset (avoid duplicate notification)
    };
}

#endif