#include "core/victim/victim_dirinfo.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimDirinfo::kClassName = "VictimDirinfo";

    VictimDirinfo::VictimDirinfo() : refcnt_(0), is_local_beaconed_(false), dirinfo_set_()
    {
        // NOTE: dirinfo set is INVALID now
    }
    
    VictimDirinfo::VictimDirinfo(const bool& is_local_beaconed) : refcnt_(0), is_local_beaconed_(is_local_beaconed), dirinfo_set_(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>())
    {
        assert(dirinfo_set_.isComplete()); // NOTE: dirinfo set in victim dirinfo MUST be complete
    }

    VictimDirinfo::VictimDirinfo(const bool& is_local_beaconed, const DirinfoSet& dirinfo_set)
    {
        refcnt_ = 0;
        is_local_beaconed_ = is_local_beaconed;
        dirinfo_set_ = dirinfo_set;

        // NOTE: as a victim MUST be cached by at least one edge node, dirinfo_set cannot be empty/invalid
        assert(dirinfo_set.isComplete()); // NOTE: dirinfo set in victim dirinfo MUST be complete
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

    const DirinfoSet& VictimDirinfo::getDirinfoSetRef() const
    {
        assert(dirinfo_set_.isComplete()); // NOTE: dirinfo set in victim dirinfo MUST be complete
        return dirinfo_set_;
    }

    void VictimDirinfo::setDirinfoSet(const DirinfoSet& dirinfo_set)
    {
        assert(dirinfo_set.isComplete()); // NOTE: dirinfo set in victim dirinfo MUST be complete
        dirinfo_set_ = dirinfo_set;
        return;
    }

    bool VictimDirinfo::updateDirinfoSet(const bool& is_admit, const DirectoryInfo& directory_info)
    {
        bool is_update = false;
        bool with_complete_dirinfo_set = false;
        if (is_admit)
        {
            with_complete_dirinfo_set = dirinfo_set_.tryToInsertIfComplete(directory_info, is_update);
        }
        else
        {
            with_complete_dirinfo_set = dirinfo_set_.tryToEraseIfComplete(directory_info, is_update);
        }
        assert(with_complete_dirinfo_set == true); // NOTE: dirinfo set in victim dirinfo MUST be complete
        
        return is_update;
    }

    uint64_t VictimDirinfo::getSizeForCapacity() const
    {
        uint64_t total_size = sizeof(uint32_t) + sizeof(bool);

        assert(dirinfo_set_.isComplete()); // NOTE: dirinfo set in victim dirinfo MUST be complete
        total_size += dirinfo_set_.getSizeForCapacity();

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