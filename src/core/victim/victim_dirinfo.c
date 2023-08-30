#include "core/victim/victim_dirinfo.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimDirinfo::kClassName = "VictimDirinfo";

    VictimDirinfo::VictimDirinfo() : refcnt_(0), is_local_beaconed_(false)
    {
        dirinfo_set_.clear();
    }

    VictimDirinfo::VictimDirinfo(const bool& is_local_beaconed, const std::set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set)
    {
        refcnt_ = 0;
        is_local_beaconed_ = is_local_beaconed;
        dirinfo_set_ = dirinfo_set;
    }

    VictimDirinfo::~VictimDirinfo() {}

    uint32_t VictimDirinfo::getRefcnt() const
    {
        return refcnt_;
    }

    void VictimDirinfo::incrRefcnt()
    {
        refcnt_++;
        return;
    }

    bool VictimDirinfo::decrRefcnt()
    {
        bool is_zero = false;
        if (refcnt_ > 0)
        {
            refcnt_--;
            if (refcnt_ == 0)
            {
                is_zero = true;
            }
        }
        else
        {
            Util::dumpWarnMsg(kClassName, "refcnt_ is already zero before decrRefcnt()!");
            is_zero = true;
        }
        return is_zero;
    }

    bool VictimDirinfo::isLocalBeaconed() const
    {
        return is_local_beaconed_;
    }

    const std::set<DirectoryInfo, DirectoryInfoHasher>& VictimDirinfo::getDirinfoSetRef() const
    {
        return dirinfo_set_;
    }

    const VictimDirinfo& VictimDirinfo::operator=(const VictimDirinfo& other)
    {
        refcnt_ = other.refcnt_;
        is_local_beaconed_ = other.is_local_beaconed_;
        dirinfo_set_ = other.dirinfo_set_;
        
        return *this;
    }
}