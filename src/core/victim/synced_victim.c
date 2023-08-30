#include "core/victim/synced_victim.h"

namespace covered
{
    const std::string SyncedVictim::kClassName = "SyncedVictim";

    SyncedVictim::SyncedVictim() : victim_info_()
    {
        dirinfo_set_.clear();
    }

    SyncedVictim::SyncedVictim(const VictimInfo& victim_info, const std::set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set)
    {
        victim_info_ = victim_info;
        dirinfo_set_ = dirinfo_set;
    }

    SyncedVictim::~SyncedVictim() {}

    const VictimInfo& SyncedVictim::getVictimInfo() const
    {
        return victim_info_;
    }

    const std::set<DirectoryInfo, DirectoryInfoHasher>& SyncedVictim::getDirinfoSet() const
    {
        return dirinfo_set_;
    }

    const SyncedVictim& SyncedVictim::operator=(const SyncedVictim& other)
    {
        victim_info_ = other.victim_info_;
        dirinfo_set_ = other.dirinfo_set_;
        
        return *this;
    }
}