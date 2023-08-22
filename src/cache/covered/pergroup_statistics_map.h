/*
 * PergroupStatisticsMap: a map of GroupLevelStatistics for each group.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef PERGROUP_STATISTICS_MAP_H
#define PERGROUP_STATISTICS_MAP_H

#include <string>
#include <unordered_map> // std::unordered_map

#include "cache/covered/common_header.h"
#include "cache/covered/group_level_statistics.h"
#include "common/key.h"

// NOTE: getSizeForCapacityWithoutKeys is used for cached objects without the size of keys; getSizeForCapacityWithKeys is used for uncached objects with the size of keys

namespace covered
{
    class PergroupStatisticsMap
    {
    public:
        PergroupStatisticsMap();
        ~PergroupStatisticsMap();

        const GroupLevelStatistics& updateForExistingKey(const GroupId& group_id, const Key& key); // Admitted cached key or tracked uncached key
        const GroupLevelStatistics& addForNewKey(const Key& key, const Value& value); // Newly admitted cached key or currently tracked uncached key
    private:
        typedef std::unordered_map<GroupId, GroupLevelStatistics> pergroup_statistics_map_t;

        static const std::string kClassName;

        GroupId assignGroupIdForNewKey_();

        GroupId cur_group_id_;
        uint32_t cur_group_keycnt_;
        pergroup_statistics_map_t pergroup_statistics_map_;
    };
}

#endif