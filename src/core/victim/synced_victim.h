/*
 * SyncedVictim: local/neighbor synced victim (may be beaconed by current edge node) including VictimInfo and DirectoryInfo.
 *
 * NOTE: the victims are synced across edge nodes by piggybacking, while extra victims will be lazily fetched by beacon node if synced victims are not enough for admission.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef SYNCED_VICTIM_H
#define SYNCED_VICTIM_H

#include <string>

#include "cooperation/directory/directory_info.h"
#include "core/victim/victim_info.h"

namespace covered
{
    class SyncedVictim
    {
    public:
        SyncedVictim();
        SyncedVictim(const VictimInfo& victim_info, const DirectoryInfo& dirinfo);
        ~SyncedVictim();

        const VictimInfo& getVictimInfo() const;
        const DirectoryInfo& getDirinfo() const;

        const SyncedVictim& operator=(const SyncedVictim& other);
    private:
        static const std::string kClassName;

        VictimInfo victim_info_;
        DirectoryInfo dirinfo_;
    };
}

#endif