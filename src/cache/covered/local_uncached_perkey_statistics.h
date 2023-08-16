/*
 * LocalUncachedPerkeyStatistics: local uncached object-level statistics used by covered local cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef LOCAL_UNCACHED_PERKEY_STATISTICS_H
#define LOCAL_UNCACHED_PERKEY_STATISTICS_H

#include <string>

#include "cache/covered/local_cached_perkey_statistics.h"

namespace covered
{
    class LocalUncachedPerkeyStatistics : public LocalCachedPerkeyStatistics
    {
    public:
        LocalUncachedPerkeyStatistics();
        ~LocalUncachedPerkeyStatistics();

        void update();

        uint32_t getRecency() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        uint32_t recency_;
    };
}

#endif