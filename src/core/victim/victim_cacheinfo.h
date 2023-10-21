/*
 * VictimCacheinfo: track key, object size, and local/redirected cached popularity of a victim.
 *
 * By Siyuan Sheng (2023.08.28).
 */

#ifndef VICTIM_CACHEINFO_H
#define VICTIM_CACHEINFO_H

#define DEBUG_VICTIM_CACHEINFO

#include <list>
#include <string>

#include "common/covered_common_header.h"
#include "common/key.h"

namespace covered
{
    class VictimCacheinfo
    {
    public:
        static VictimCacheinfo dedup(const VictimCacheinfo& current_victim_cacheinfo, const VictimCacheinfo& prev_victim_cacheinfo);
        static VictimCacheinfo recover(const VictimCacheinfo& compressed_victim_cacheinfo, const VictimCacheinfo& existing_victim_cacheinfo);

        static void sortByLocalRewards(std::list<VictimCacheinfo>& victim_cacheinfos); // Sort victim cacheinfos by local rewards (ascending order)

        VictimCacheinfo();
        VictimCacheinfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const Reward& local_reward);
        ~VictimCacheinfo();

        bool isComplete() const; // Whether all fields are NOT deduped
        bool isStale() const; // Whether the given key is NOT local synced victim yet and should be removed from victim tracker
        bool isDeduped() const; // Whether at least one field is deduped
        bool isFullyDeduped() const; // Whether all fields are deduped (i.e., no need for victim synchronization)

        // For complete victim cacheinfo
        const Key getKey() const;
        bool getObjectSize(ObjectSize& object_size) const; // Return if with complete object size
        bool getLocalCachedPopularity(Popularity& local_cached_popularity) const; // Return if with complete local cached popularity
        bool getRedirectedCachedPopularity(Popularity& redirected_cached_popularity) const; // Return if with complete redirected cached popularity
        bool getLocalReward(Reward& local_reward) const; // Return if with complete local reward

        // For compressed victim cacheinfo
        void markStale(); // Dedup all fields
        void dedupObjectSize();
        void dedupLocalCachedPopularity();
        void dedupRedirectedCachedPopularity();
        void dedupLocalReward();

        uint32_t getVictimCacheinfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const VictimCacheinfo& operator=(const VictimCacheinfo& other);
        bool operator<(const VictimCacheinfo& other) const;
    private:
        static const std::string kClassName;

        static const uint8_t INVALID_BITMAP; // Invalid bitmap
        static const uint8_t COMPLETE_BITMAP; // All fields are NOT deduped
        static const uint8_t DEDUP_MASK; // At least one field is deduped
        static const uint8_t STALE_BITMAP; // The given key is NOT local synced victim yet and should be removed from victim tracker
        static const uint8_t OBJECT_SIZE_DEDUP_MASK; // Whether object size is deduped or complete
        static const uint8_t LOCAL_CACHED_POPULARITY_DEDUP_MASK; // Whether local cached popularity is deduped or complete
        static const uint8_t REDIRECTED_CACHED_POPULARITY_DEDUP_MASK; // Whether redirected cached popularity is deduped or complete
        static const uint8_t LOCAL_REWARD_DEDUP_MASK; // Whether local reward is deduped or complete

        uint8_t dedup_bitmap_; // 1st lowest bit indicates if the cacheinfo is a compressed victim cacheinfo (2nd lowest bit for object size; 3rd lowest bit for local cached popularity; 4th lowest bit for redirected cached popularity; 5th lowest bit for local reward)
        Key key_;

        // For both complete and compressed victim cacheinfo
        ObjectSize object_size_;
        Popularity local_cached_popularity_; // Calculated by local edge cache based on local cache hits
        Popularity redirected_cached_popularity_; // Calculated by local edge cache based on redirected cache hits
        Reward local_reward_; // Calculated by local edge cache based on local/redirected cached popularity
    };

    class VictimCacheinfoCompare
    {
    public:
        bool operator()(const VictimCacheinfo& lhs, const VictimCacheinfo& rhs) const;
    };
}

#endif