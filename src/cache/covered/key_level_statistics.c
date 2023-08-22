#include "cache/covered/key_level_statistics.h"

namespace covered
{
    const std::string KeyLevelStatistics::kClassName("KeyLevelStatistics");

    KeyLevelStatistics::KeyLevelStatistics()
    {
        group_id_ = 0;
        frequency_ = 0;
    }

    KeyLevelStatistics::~KeyLevelStatistics() {}

    void KeyLevelStatistics::update()
    {
        frequency_++;
        return;
    }

    GroupId KeyLevelStatistics::getGroupId() const
    {
        return group_id_;
    }

    uint32_t KeyLevelStatistics::getFrequency() const
    {
        return frequency_;
    }
}