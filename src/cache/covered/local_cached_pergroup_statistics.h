/*
 * LocalCachedPergroupStatistics: local cached group-level statistics used by covered local cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef LOCAL_CACHED_PERGROUP_STATISTICS_H
#define LOCAL_CACHED_PERGROUP_STATISTICS_H

#include <string>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class LocalCachedPergroupStatistics
    {
    public:
        LocalCachedPergroupStatistics();
        ~LocalCachedPergroupStatistics();

        void updateForNewlyGroupedKey(const Key& key, const Value& value);
        void updateForInGroupKey();
        void updateForDegroupedKey();

        uint32_t getObjectSize() const;
        uint32_t getObjectCnt() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later
        uint32_t avg_object_size_;
        uint32_t object_cnt_;
    };
}

#endif