#include "cache/covered/key_level_metadata.h"

namespace covered
{
    const std::string KeyLevelMetadata::kClassName("KeyLevelMetadata");

    KeyLevelMetadata::KeyLevelMetadata(const GroupId& group_id) : group_id_(group_id)
    {
        frequency_ = 0;
    }

    KeyLevelMetadata::~KeyLevelMetadata() {}

    void KeyLevelMetadata::updateDynamicMetadata()
    {
        frequency_++;
        return;
    }

    GroupId KeyLevelMetadata::getGroupId() const
    {
        return group_id_;
    }

    uint32_t KeyLevelMetadata::getFrequency() const
    {
        return frequency_;
    }
}