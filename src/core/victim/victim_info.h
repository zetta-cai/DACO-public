/*
 * VicimInfo: track key, local/redirected cached popularity, and DirectoryInfo of a victim.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_INFO_H
#define VICTIM_INFO_H

#include <string>

#include "cache/covered/common_header.h"
#include "common/key.h"
#include "cooperation/directory/directory_info.h"

namespace covered
{
    class VictimInfo
    {
    public:
        VictimInfo();
        VictimInfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const DirectoryInfo& dirinfo);
        ~VictimInfo();

        const Key& getKey() const;
        const ObjectSize& getObjectSize() const;
        const Popularity& getLocalCachedPopularity() const;
        const Popularity& getRedirectedCachedPopularity() const;
        const DirectoryInfo& getDirinfo() const;

        const VictimInfo& operator=(const VictimInfo& other);
    private:
        static const std::string kClassName;

        Key key_;
        ObjectSize object_size_;
        Popularity local_cached_popularity_;
        Popularity redirected_cached_popularity_;
        DirectoryInfo dirinfo_;
    };
}

#endif