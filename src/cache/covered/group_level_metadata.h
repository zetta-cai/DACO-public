/*
 * GroupLevelMetadata: group-level metadata for each object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef GROUP_LEVEL_METADATA_H
#define GROUP_LEVEL_METADATA_H

#include <string>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class GroupLevelMetadata
    {
    public:
        GroupLevelMetadata();
        GroupLevelMetadata(const GroupLevelMetadata& other);
        ~GroupLevelMetadata();

        void updateForNewlyGrouped(const Key& key, const Value& value); // Update group-level metadata for the key newly added into the current group (newly admitted/tracked for local cached/uncached)
        void updateForInGroupKey(const Key& key); // Update group-level metadata for the key already in the current group; NOT affect value-related metadata (getreq-hit/getrsp-miss of admitted/tracked objects for local cached/uncached)
        void updateForInGroupKeyValue(const Key& key, const Value& value, const Value& original_value); // Update group-level metadata for the key already in the current group (putdelreq-hit/putdelrsp-miss of admitted/tracked objects for local cached/uncached)
        bool updateForDegrouped(const Key& key, const Value& original_value); // Update group-level metadata for the key being removed from the current group (currently evicted/detracked for local cached/uncached); return true if object_cnt_ is zero (i.e., all keys in the group have been removed and the group can also be removed)

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