/*
 * PerkeyStatisticsList: a linked list of KeyLevelStatistics for each key.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef PERKEY_STATISTICS_LIST_H
#define PERKEY_STATISTICS_LIST_H

#include <string>
#include <list> // std::list

#include "cache/covered/key_level_statistics.h"
#include "common/key.h"

// NOTE: getSizeForCapacityWithoutKeys is used for cached objects without the size of keys; getSizeForCapacityWithKeys is used for uncached objects with the size of keys

namespace covered
{
    class PerkeyStatisticsList
    {
    public:
        PerkeyStatisticsList();
        ~PerkeyStatisticsList();
    private:
        typedef std::list<std::pair<Key, KeyLevelStatistics>> perkey_statistics_list_t;

        static const std::string kClassName;

        perkey_statistics_list_t perkey_statistics_list_;
    };
}

#endif