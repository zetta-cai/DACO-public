/*
 * VictimDirinfo: track refcnt, is local beaconed flag, and a set of DirectoryInfos of a victim.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_DIRINFO_H
#define VICTIM_DIRINFO_H

#include <string>

#include "cooperation/directory/directory_info.h"

namespace covered
{
    class VictimDirinfo
    {
    public:
        VictimDirinfo();
        VictimDirinfo(const bool& is_local_beaconed, const dirinfo_set_t& dirinfo_set);
        ~VictimDirinfo();

        uint32_t getRefcnt() const;
        void incrRefcnt();
        bool decrRefcnt(); // Return true if refcnt_ is 0 after decrement

        bool isLocalBeaconed() const;
        void markLocalBeaconed();

        const dirinfo_set_t& getDirinfoSetRef() const;
        void setDirinfoSet(const dirinfo_set_t& dirinfo_set);
        void updateDirinfo(const bool& is_admit, const DirectoryInfo& directory_info); // Update directory info for the key (i.e., add or remove directory info from dirinfo_set_)

        const VictimDirinfo& operator=(const VictimDirinfo& other);
    private:
        static const std::string kClassName;

        uint32_t refcnt_;
        bool is_local_beaconed_;
        dirinfo_set_t dirinfo_set_;
    };
}

#endif