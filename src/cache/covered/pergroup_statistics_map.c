#include "cache/covered/pergroup_statistics_map.h"

namespace covered
{
    const std::string PergroupStatisticsMap::kClassName("PergroupStatisticsMap");

    PergroupStatisticsMap::PergroupStatisticsMap()
    {
        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_statistics_map_.clear();
    }
    
    PergroupStatisticsMap::~PergroupStatisticsMap() {}

    const GroupLevelStatistics& PergroupStatisticsMap::updateForExistingKey(const GroupId& group_id, const Key& key)
    {
        pergroup_statistics_map_t::iterator pergroup_statistics_iter = pergroup_statistics_map_.find(group_id);
        assert(pergroup_statistics_iter != pergroup_statistics_map_.end());
        pergroup_statistics_iter->second.updateForInGroupKey(key);

        return pergroup_statistics_iter->second;
    }

    const GroupLevelStatistics& PergroupStatisticsMap::addForNewKey(const Key& key, const Value& value)
    {
        GroupId group_id = assignGroupIdForNewKey_();
        pergroup_statistics_map_t::iterator pergroup_statistics_iter = pergroup_statistics_map_.insert(std::pair(group_id, GroupLevelStatistics())).first;
        assert(pergroup_statistics_iter != pergroup_statistics_map_.end());
        pergroup_statistics_iter->second.updateForNewlyGrouped(key, value);

        return pergroup_statistics_iter->second;
    }

    GroupId PergroupStatisticsMap::assignGroupIdForNewKey_()
    {
        cur_group_keycnt_++;
        if (cur_group_keycnt_ > COVERED_PERGROUP_MAXKEYCNT)
        {
            cur_group_id_++;
            cur_group_keycnt_ = 1;
        }     

        return cur_group_id_;
    }
}