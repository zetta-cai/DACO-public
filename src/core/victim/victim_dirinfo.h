/*
 * VictimDirinfo: track refcnt, is local beaconed flag, and a set of DirectoryInfos of a victim.
 *
 * NOTE: all dirinfo sets stored in each edge node (including local directory table, victim dirinfo, extra fetched victims, and victim metadata cache of victim tracker) MUST be complete (ONLY transmitted dirinfo sets could be compressed).
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_DIRINFO_H
#define VICTIM_DIRINFO_H

#include <string>

#include "cooperation/directory/dirinfo_set.h"

namespace covered
{
    class VictimDirinfo
    {
    public:
        VictimDirinfo();
        //VictimDirinfo(const bool& is_local_beaconed);
        //VictimDirinfo(const bool& is_local_beaconed, const DirinfoSet& dirinfo_set);
        VictimDirinfo(const uint32_t& beacon_edge_idx);
        VictimDirinfo(const uint32_t& beacon_edge_idx, const DirinfoSet& dirinfo_set);
        ~VictimDirinfo();

        uint32_t getRefcnt() const;
        void incrRefcnt();
        bool decrRefcnt(); // Return true if refcnt_ is 0 after decrement

        //bool isLocalBeaconed() const;
        //void markLocalBeaconed();

        uint32_t getBeaconEdgeIdx() const;
        void setBeaconEdgeIdx(const uint32_t& beacon_edge_idx);

        const DirinfoSet& getDirinfoSetRef() const;
        void setDirinfoSet(const DirinfoSet& dirinfo_set);
        bool updateDirinfoSet(const bool& is_admit, const DirectoryInfo& directory_info); // Update directory info for the key (i.e., add or remove directory info from dirinfo_set_); return if directory info set is updated successfully

        uint64_t getSizeForCapacity() const;

        const VictimDirinfo& operator=(const VictimDirinfo& other);
    private:
        static const std::string kClassName;

        uint32_t refcnt_;
        //bool is_local_beaconed_;
        uint32_t beacon_edge_idx_; // NOTE: used by VictimTracker get victim syncset of a specific edge idx for delta recovery of delta-based victim synchronization in CoveredCacheManager (outside VictimTracker)
        DirinfoSet dirinfo_set_;
    };
}

#endif