#include "core/victim/victimsync_monitor.h"

namespace covered
{
    const std::string VictimsyncMonitor::kClassName = "VictimsyncMonitor";

    VictimsyncMonitor::VictimsyncMonitor() : cur_seqnum_(0), prev_victim_syncset_ptr_(nullptr), need_enforcement_(false), is_first_received_(true), tracked_seqnum_(0), enforcement_seqnum_(0), wait_for_complete_victim_syncset_(false)
    {
        cached_victim_syncsets_.clear();
    }

    // As receiver edge node

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
}