/*
 * LocalCachedMetadata: metadata for local cached objects in covered.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_CACHED_METADATA_H
#define LOCAL_CACHED_METADATA_H

#include <string>

#include "cache/covered/cache_metadata_base.h"

namespace covered
{
    class LocalCachedMetadata : public CacheMetadataBase
    {
    public:
        LocalCachedMetadata();
        virtual ~LocalCachedMetadata();

        // ONLY for local cached objects

        bool getLeastPopularKeyObjsizePopularity(const uint32_t& least_popular_rank, Key& key, ObjectSize& object_size, Popularity& local_cached_popularity, Popularity& redirected_cached_popularity, Reward& local_reward) const; // Get ith least popular key and its local/redirected cached popularity for local cached object with local reward (determine the ranking order)

        // Different for local cached objects

        // For reward information
        virtual Reward calculateReward_(const Popularity& local_popularity, const Popularity& redirected_popularity) const override;

        // All the following functions return if affect local synced victims in victim tracker
        bool addForNewKey(const Key& key, const Value& value, const uint32_t& peredge_synced_victimcnt); // For admission, initialize and update both value-unrelated and value-related metadata for newly-admited key
        bool updateNoValueStatsForExistingKey(const Key& key, const uint32_t& peredge_synced_victimcnt); // For get/put/delreq w/ hit, update object-/group-level value-unrelated metadata for existing key (i.e., already admitted objects for local cached)
        bool updateValueStatsForExistingKey(const Key& key, const Value& value, const Value& original_value, const uint32_t& peredge_synced_victimcnt); // For put/delreq w/ hit and getrsp w/ invalid-hit, update object-/group-level value-related metadata for existing key (i.e., already admitted objects for local cached)

        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove admitted cached key (for eviction)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local cached objects
    private:
        static const std::string kClassName;
    };
}

#endif