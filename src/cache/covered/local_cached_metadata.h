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

        bool getLeastPopularKeyAndPopularity(const uint32_t& least_popular_rank, Key& key, Popularity& local_cached_popularity, Popularity& redirected_cached_popularity) const; // Get ith least popular key and its local/redirected cached popularity for local cached object

        // Different for local cached objects

        // Return if affect local synced victims in victim tracker
        bool addForNewKey(const Key& key, const Value& value, const uint32_t& peredge_synced_victimcnt); // Newly admitted cached key (for admission); overwrite CacheMetadataBase to return if affect local synced victims in victim tracker
        bool updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related, const uint32_t& peredge_synced_victimcnt); // Admitted cached key (is_value_related = false: for getreq with cache hit; is_value_related = true: for getrsp with invalid hit, put/delreq with cache hit); overwrite CacheMetadataBase to return if affect local synced victims in victim tracker

        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove admitted cached key (for eviction)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local cached objects
    private:
        static const std::string kClassName;
    };
}

#endif