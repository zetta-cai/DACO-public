/*
 * PergroupStatistics: local cached group-level statistics used by covered local cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef PERGROUP_STATISTICS_H
#define PERGROUP_STATISTICS_H

#include <string>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class PergroupStatistics
    {
    public:
        PergroupStatistics();
        ~PergroupStatistics();

        void updateForNewlyGrouped(const Key& key, const Value& value); // Update group-level statistics for the key newly added into the current group (newly admitted/tracked for local cached/uncached)
        void updateForInGroupKey(const Key& key); // Update group-level statistics for the key already in the current group; NOT affect value-related statistics (getreq-hit/getrsp-miss of admitted/tracked objects for local cached/uncached)
        void updateForInGroupKeyValue(const Key& key, const Value& value, const Value& original_value); // Update group-level statistics for the key already in the current group (putdelreq-hit/putdelrsp-miss of admitted/tracked objects for local cached/uncached)
        void updateForDegrouped(const Key& key, const Value& original_value); // Update group-level statistics for the key being removed from the current group (currently evicted/detracked for local cached/uncached)

        uint32_t getAvgObjectSize() const;
        uint32_t getObjectCnt() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        uint32_t avg_object_size_;
        uint32_t object_cnt_;
    };
}

#endif