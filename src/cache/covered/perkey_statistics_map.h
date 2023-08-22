/*
 * PerkeyStatisticsMap: a map of KeyLevelStatistics for each key.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef PERKEY_STATISTICS_MAP_H
#define PERKEY_STATISTICS_MAP_H

#include <string>
#include <unordered_map> // std::unordered_map

#include "cache/covered/common_header.h"
#include "cache/covered/key_level_statistics.h"
#include "common/key.h"

// NOTE: getSizeForCapacityWithoutKeys is used for cached objects without the size of keys; getSizeForCapacityWithKeys is used for uncached objects with the size of keys

namespace covered
{
    class PerkeyStatisticsMap
    {
    public:
        PerkeyStatisticsMap();
        ~PerkeyStatisticsMap();

        const KeyLevelStatistics& updateForExistingKey(const Key& key); // Admitted cached key or tracked uncached key

        GroupId getGroupIdForExistingKey(const Key& key) const;
    private:
        typedef std::unordered_map<Key, KeyLevelStatistics, KeyHasher> perkey_statistics_map_t;

        static const std::string kClassName;

        perkey_statistics_map_t perkey_statistics_map_;
    };
}

#endif