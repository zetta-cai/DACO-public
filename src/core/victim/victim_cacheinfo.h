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

        bool isComplete() const; // Whether all fields are NOT deduped
        bool isStale() const; // Whether the given key is NOT local synced victim yet and should be removed from victim tracker
        bool isDeduped() const; // Whether at least one field is deduped (but NOT all fields)

        const Key getKey() const;
        bool getObjectSize(ObjectSize& object_size) const; // Return if object size exstis
        bool getLocalCachedPopularity(Popularity& local_cached_popularity) const; // Return if local cached popularity exists
        bool getRedirectedCachedPopularity(Popularity& redirected_cached_popularity) const; // Return if redirected cached popularity exists

        uint32_t getVictimCacheinfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const VictimCacheinfo& operator=(const VictimCacheinfo& other);
    private:
        static const std::string kClassName;

        // NOTE: as we do NOT need to sync victim cache info if all three fields are deduped, so we use dedup_bitmap_ = STALE_BITMAP (i.e., all three lowest bits are 1) to indicate that the stale victim cacheinfo of key_ needs to be removed (i.e., key_ is NOT a local synced victim)
        static const uint8_t COMPLETE_BITMAP; // All fields are NOT deduped
        static const uint8_t STALE_BITMAP; // The given key is NOT local synced victim yet and should be removed from victim tracker
        static const uint8_t OBJECT_SIZE_DEDUP_MASK; // Whether object size is deduped or complete
        static const uint8_t LOCAL_CACHED_POPULARITY_DEDUP_MASK; // Whether local cached popularity is deduped or complete
        static const uint8_t REDIRECTED_CACHED_POPULARITY_DEDUP_MASK; // Whether redirected cached popularity is deduped or complete

        uint8_t dedup_bitmap_; // Whether the cacheinfo is a compressed victim cacheinfo (1st lowest bit for object size; 2nd lowest bit for local cached popularity; 3rd lowest bit for redirected cached popularity)
        Key key_;

        ObjectSize object_size_;
        Popularity local_cached_popularity_;
        Popularity redirected_cached_popularity_;
    };
}

#endif