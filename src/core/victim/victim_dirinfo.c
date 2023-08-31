#include "core/victim/victim_dirinfo.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimDirinfo::kClassName = "VictimDirinfo";

    VictimDirinfo::VictimDirinfo() : refcnt_(0), is_local_beaconed_(false)
    {
        dirinfo_set_.clear();
    }

    VictimDirinfo::VictimDirinfo(const bool& is_local_beaconed, const dirinfo_set_t& dirinfo_set)
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

    void VictimDirinfo::markLocalBeaconed()
    {
        is_local_beaconed_ = true;
        return;
    }

    const dirinfo_set_t& VictimDirinfo::getDirinfoSetRef() const
    {
        return dirinfo_set_;
    }

    void VictimDirinfo::setDirinfoSet(const dirinfo_set_t& dirinfo_set)
    {
        dirinfo_set_ = dirinfo_set;
        return;
    }

    bool VictimDirinfo::updateDirinfoSet(const bool& is_admit, const DirectoryInfo& directory_info)
    {
        bool is_update = false;
        
        if (is_admit && dirinfo_set_.find(directory_info) == dirinfo_set_.end())
        {
            dirinfo_set_.insert(directory_info);
            is_update = true;
        }
        else if (!is_admit && dirinfo_set_.find(directory_info) != dirinfo_set_.end())
        {
            dirinfo_set_.erase(directory_info);
            is_update = true;
        }
        
        return is_update;
    }

    uint64_t VictimDirinfo::getSizeForCapacity() const
    {
        uint64_t total_size = sizeof(uint32_t) + sizeof(bool);

        for (dirinfo_set_t::const_iterator dirinfo_iter = dirinfo_set_.begin(); dirinfo_iter != dirinfo_set_.end(); ++dirinfo_iter)
        {
            total_size += dirinfo_iter->getSizeForCapacity();
        }

        return total_size;
    }

    const VictimDirinfo& VictimDirinfo::operator=(const VictimDirinfo& other)
    {
        refcnt_ = other.refcnt_;
        is_local_beaconed_ = other.is_local_beaconed_;
        dirinfo_set_ = other.dirinfo_set_;
        
        return *this;
    }
}