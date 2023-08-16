#include "cache/covered/local_cached_perkey_statistics.h"

namespace covered
{
    const std::string LocalCachedPerkeyStatistics::kClassName("LocalCachedPerkeyStatistics");

    LocalCachedPerkeyStatistics::LocalCachedPerkeyStatistics()
    {
        group_id_ = 0;
        frequency_ = 0;
    }

    LocalCachedPerkeyStatistics::~LocalCachedPerkeyStatistics() {}

    void LocalCachedPerkeyStatistics::update()
    {
        frequency_++;
        return;
    }

    uint32_t LocalCachedPerkeyStatistics::getGroupId() const
    {
        return group_id_;
    }

    uint32_t LocalCachedPerkeyStatistics::getFrequency() const
    {
        return frequency_;
    }
}