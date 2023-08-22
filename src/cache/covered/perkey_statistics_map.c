#include "cache/covered/perkey_statistics_map.h"

#include <assert.h>

namespace covered
{
    const std::string PerkeyStatisticsMap::kClassName("PerkeyStatisticsMap");

    PerkeyStatisticsMap::PerkeyStatisticsMap()
    {
        perkey_statistics_map_.clear();
    }
    
    PerkeyStatisticsMap::~PerkeyStatisticsMap() {}

    const KeyLevelStatistics& PerkeyStatisticsMap::updateForExistingKey(const Key& key)
    {
        perkey_statistics_map_t::iterator perkey_statistics_iter = perkey_statistics_map_.find(key);
        assert(perkey_statistics_iter != perkey_statistics_map_.end());
        perkey_statistics_iter->second.update();

        return perkey_statistics_iter->second;
    }

    GroupId PerkeyStatisticsMap::getGroupIdForExistingKey(const Key& key) const
    {
        perkey_statistics_map_t::const_iterator iter = perkey_statistics_map_.find(key);
        assert(iter != perkey_statistics_map_.end()); // key must be admitted before

        return iter->second.getGroupId();
    }
}