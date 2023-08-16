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

        void updateForNewlyGroupedKey(const Key& key, const Value& value); // Update group-level statistics for the key newly added into the current group
        void updateForInGroupKey(const Key& key); // Update group-level statistics for the key already in the current group (NOT affect value-related statistics)
        void updateForInGroupKey(const Key& key, const Value& value, const Value& original_value); // Update group-level statistics for the key already in the current group
        void updateForDegroupedKey(const Key& key, const Value& original_value); // Update group-level statistics for the key being removed from the current group

        uint32_t getObjectSize() const;
        uint32_t getObjectCnt() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        uint32_t avg_object_size_;
        uint32_t object_cnt_;
    };
}

#endif