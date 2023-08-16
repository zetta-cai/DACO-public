#include "cache/covered/local_uncached_perkey_statistics.h"

namespace covered
{
    const std::string LocalUncachedPerkeyStatistics::kClassName("LocalUncachedPerkeyStatistics");

    LocalUncachedPerkeyStatistics::LocalUncachedPerkeyStatistics() : LocalCachedPerkeyStatistics()
    {
        recency_ = 0;
    }

    LocalUncachedPerkeyStatistics::~LocalUncachedPerkeyStatistics() {}

    void LocalUncachedPerkeyStatistics::update()
    {
        LocalCachedPerkeyStatistics::update();
        
        return;
    }

    uint32_t LocalUncachedPerkeyStatistics::getRecency() const
    {
        return recency_;
    }
}