/*
 * LocalCachedPerkeyStatistics: local cached object-level statistics used by covered local cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef LOCAL_CACHED_PERKEY_STATISTICS_H
#define LOCAL_CACHED_PERKEY_STATISTICS_H

#include <string>

namespace covered
{
    class LocalCachedPerkeyStatistics
    {
    public:
        LocalCachedPerkeyStatistics();
        ~LocalCachedPerkeyStatistics();

        void update();

        uint32_t getGroupId() const;
        uint32_t getFrequency() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        uint32_t group_id_;
        uint32_t frequency_;
    };
}

#endif