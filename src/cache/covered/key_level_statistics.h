/*
 * KeyLevelStatistics: key-level statistics for each object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef KEY_LEVEL_STATISTICS_H
#define KEY_LEVEL_STATISTICS_H

#include <string>

#include "cache/covered/common_header.h"

namespace covered
{
    class KeyLevelStatistics
    {
    public:
        KeyLevelStatistics();
        ~KeyLevelStatistics();

        void update();

        GroupId getGroupId() const;
        uint32_t getFrequency() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        GroupId group_id_;
        uint32_t frequency_;
    };
}

#endif