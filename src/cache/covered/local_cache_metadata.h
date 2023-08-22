/*
 * LocalCacheMetadata: metadata for covered local cache (including object-level, group-level and sorted popularity metadata).
 * 
 * By Siyuan Sheng (2023.08.22).
 */

#ifndef LOCAL_CACHE_METADATA_H
#define LOCAL_CACHE_METADATA_H

#include <list> // std::list
#include <map> // std::multimap
#include <string>
#include <unordered_map> // std::unordered_map

#include "cache/covered/common_header.h"
#include "cache/covered/key_level_metadata.h"
#include "cache/covered/group_level_metadata.h"
#include "common/key.h"

// TODO: as we have counted the size of LRU list and lookup table for cached objects in cachelib engine, we do NOT count it again

// NOTE: getSizeForCapacityWithoutKeys is used for cached objects without the size of keys; getSizeForCapacityWithKeys is used for uncached objects with the size of keys

namespace covered
{
    typedef std::list<std::pair<Key, KeyLevelMetadata>> perkey_metadata_list_t;
    typedef std::unordered_map<GroupId, GroupLevelMetadata> pergroup_metadata_map_t;
    //typedef std::multimap<Popularity, LruCacheReadHandle> sorted_popularity_multimap_t; // Obselete: local uncached objects cannot provide LruCacheReadHandle
    class perkey_lookup_iter_t; // Forward declaration
    typedef std::multimap<Popularity, perkey_lookup_iter_t> sorted_popularity_multimap_t;

    class LookupMetadata
    {
    public:
        LookupMetadata();
        ~LookupMetadata();

        perkey_metadata_list_t::iterator getPerkeyMetadataIter() const;
        sorted_popularity_multimap_t::iterator getSortedPopularityIter() const;

        void setPerkeyMetadataIter(const perkey_metadata_list_t::iterator& perkey_metadata_iter);
        void setSortedPopularityIter(const sorted_popularity_multimap_t::iterator& sorted_popularity_iter);
    private:
        static const std::string kClassName;

        perkey_metadata_list_t::iterator perkey_metadata_iter_;
        sorted_popularity_multimap_t::iterator sorted_popularity_iter_;
    };

    class LocalCacheMetadata
    {
    public:
        LocalCacheMetadata();
        ~LocalCacheMetadata();

        bool isKeyExist(const Key& key) const; // Check if key has been admitted or tracked for local cached or uncached object

        void addForNewKey(const Key& key, const Value& value); // Newly admitted cached key or currently tracked uncached key
        void updateForExistingKey(const Key& key); // Admitted cached key or tracked uncached key (triggered by getreq with cache hits, getrsp for cache misses)
    private:
        typedef std::unordered_map<Key, LookupMetadata, KeyHasher> perkey_lookup_table_t;
        typedef perkey_lookup_table_t::iterator perkey_lookup_iter_t;

        static const std::string kClassName;

        // For group-level metadata
        GroupId assignGroupIdForNewKey_();

        // For popularity information
        Popularity calculatePopularity_(const KeyLevelMetadata& key_level_statistics, const GroupLevelMetadata& group_level_statistics) const; // Calculate popularity based on object-level and group-level metadata
        void updatePopularity_(const sorted_popularity_multimap_t::iterator& old_sorted_popularity_iter, const Popularity& new_popularity, perkey_lookup_iter_t& perkey_lookup_iter); // Return new sorted popularity iterator
        
        // Object-level metadata
        perkey_metadata_list_t perkey_metadata_list_; // LRU list for object-level metadata (list index is recency information)

        // Group-level metadata
        GroupId cur_group_id_;
        uint32_t cur_group_keycnt_;
        pergroup_metadata_map_t pergroup_metadata_map_; // Group-level metadata (grouping based on admission/tracked time)

        // Popularity information
        sorted_popularity_multimap_t sorted_popularity_multimap_; // Sorted popularity information (ascending order; allow duplicate popularity values)

        // Lookup table
        perkey_lookup_table_t perkey_lookup_table_;
    };
}

#endif