/*
 * SyncedVictim: local/neighbor synced victim (may be beaconed by current edge node) including VictimInfo and DirectoryInfo.
 *
 * NOTE: the victims are synced across edge nodes by piggybacking, while extra victims will be lazily fetched by beacon node if synced victims are not enough for admission.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef SYNCED_VICTIM_H
#define SYNCED_VICTIM_H

#include <set>
#include <string>

#include "cooperation/directory/directory_info.h"
#include "core/victim/victim_info.h"

namespace covered
{
    class SyncedVictim
    {
    public:
        SyncedVictim();
        SyncedVictim(const VictimInfo& victim_info, const std::set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set);
        ~SyncedVictim();

        const VictimInfo& getVictimInfo() const;
        const std::set<DirectoryInfo, DirectoryInfoHasher>& getDirinfoSet() const;

        const SyncedVictim& operator=(const SyncedVictim& other);
    private:
        static const std::string kClassName;

        VictimInfo victim_info_;
        std::set<DirectoryInfo, DirectoryInfoHasher> dirinfo_set_;
    };
}

#endif