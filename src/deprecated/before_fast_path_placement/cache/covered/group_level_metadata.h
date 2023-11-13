/*
 * GroupLevelMetadata: group-level metadata for each object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef GROUP_LEVEL_METADATA_H
#define GROUP_LEVEL_METADATA_H

#include <string>

#include "common/covered_common_header.h"
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

        void updateForNewlyGrouped(const Key& key, const Value& value); // For admission and getrsp/put/delreq w/ miss (also getreq w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), update group-level metadata (both value-unrelated and value-related) for the key newly added into the current group (newly admitted/tracked for local cached/uncached)
        void updateNoValueStatsForInGroupKey(); // For get/put/delreq w/ hit/miss, update group-level value-unrelated metadata for the key already in the current group (i.e., already admitted/tracked objects for local cached/uncached)
        void updateValueStatsForInGroupKey(const Key& key, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit (also getrsp w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), update group-level value-related metadata for the key already in the current group (i.e., already admitted/tracked objects for local cached/uncached)
        bool updateForDegrouped(const Key& key, const Value& value, const bool& need_warning); // Update group-level metadata for the key being removed from the current group (currently evicted/detracked for local cached/uncached); return true if object_cnt_ is zero (i.e., all keys in the group have been removed and the group can also be removed)

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        AvgObjectSize getAvgObjectSize() const;
        #endif
        ObjectCnt getObjectCnt() const;

        static uint64_t getSizeForCapacity();
    private:
        static const std::string kClassName;

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        // Value-related metadata
        AvgObjectSize avg_object_size_;
        #endif

        // Value-unrelated metadata
        ObjectCnt object_cnt_; // ONLY for newly-grouped or degrouped keys
    };
}

#endif