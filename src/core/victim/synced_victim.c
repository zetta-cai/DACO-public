#include "core/victim/synced_victim.h"

namespace covered
{
    const std::string SyncedVictim::kClassName = "SyncedVictim";

    SyncedVictim::SyncedVictim() : victim_info_(), dirinfo_()
    {
    }

    SyncedVictim::SyncedVictim(const VictimInfo& victim_info, const DirectoryInfo& dirinfo)
    {
        victim_info_ = victim_info;
        dirinfo_ = dirinfo;
    }

    SyncedVictim::~SyncedVictim() {}

    const VictimInfo& SyncedVictim::getVictimInfo() const
    {
        return victim_info_;
    }

    const DirectoryInfo& SyncedVictim::getDirinfo() const
    {
        return dirinfo_;
    }

    const SyncedVictim& SyncedVictim::operator=(const SyncedVictim& other)
    {
        victim_info_ = other.victim_info_;
        dirinfo_ = other.dirinfo_;
        
        return *this;
    }
}