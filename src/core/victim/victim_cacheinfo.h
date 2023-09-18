/*
 * VictimCacheinfo: track key, object size, and local/redirected cached popularity of a victim.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_CACHEINFO_H
#define VICTIM_CACHEINFO_H

#include <string>

#include "common/covered_common_header.h"
#include "common/key.h"

namespace covered
{
    class VictimCacheinfo
    {
    public:
        VictimCacheinfo();
        VictimCacheinfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity);
        ~VictimCacheinfo();

        const Key getKey() const;
        const ObjectSize getObjectSize() const;
        const Popularity getLocalCachedPopularity() const;
        const Popularity getRedirectedCachedPopularity() const;

        uint32_t getVictimCacheinfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const VictimCacheinfo& operator=(const VictimCacheinfo& other);
    private:
        static const std::string kClassName;

        Key key_;
        ObjectSize object_size_;
        Popularity local_cached_popularity_;
        Popularity redirected_cached_popularity_;
    };
}

#endif