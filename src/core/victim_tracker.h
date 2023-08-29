/*
 * VictimTracker: track peredge_synced_victimcnt VictimInfos with the least local rewards (w1 * local cached popularity + w2 * redirected cached popularity) for each edge node (thread safe).
 *
 * NOTE: we call the synced victims from the current edge node as local synced victims, while those from other edge nodes as neighbor synced victims. A part of local/neighbor synced victims are beaconed by the current edge node.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_TRACKER_H
#define VICTIM_TRACKER_H

#include <string>
#include <unordered_map>
#include <list>

#include "concurrency/rwlock.h"
#include "core/victim/synced_victim.h"

namespace covered
{
    class VictimTracker
    {
    public:
        VictimTracker(const uint32_t& edge_idx, const uint32_t& peredge_synced_victimcnt);
        ~VictimTracker();

        void updateLocalSyncedVictimInfos(const std::list<VictimInfo>& local_synced_victim_infos);
    private:
        // NOTE: the list of SyncedVictims follows the ascending order of local rewards
        typedef std::unordered_map<uint32_t, std::list<SyncedVictim>> peredge_synced_victims_t;

        static const std::string kClassName;

        void checkPointers_() const;

        // Const shared variables
        const uint32_t edge_idx_;
        std::string instance_name_;
        const uint32_t peredge_synced_victimcnt_;

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as VictimTracker's data structures have two types of keys (edge index and Key)
        mutable Rwlock* rwlock_for_victim_tracker_;

        // Non-const shared varaibles
        // TODO: Maintain per-edge-node margin bytes to decide whether to perform placement calculation or not
        peredge_synced_victims_t peredge_synced_victims_;
    };
}

#endif