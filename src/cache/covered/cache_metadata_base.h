/*
 * CacheMetadataBase: metadata for covered local cache (including object-level, group-level and sorted popularity metadata).
 * 
 * By Siyuan Sheng (2023.08.22).
 */

#ifndef CACHE_METADATA_BASE_H
#define CACHE_METADATA_BASE_H

#include <iterator> // std::distance
#include <list> // std::list
#include <map> // std::multimap
#include <string>
#include <unordered_map> // std::unordered_map

#include "common/covered_common_header.h"
#include "cache/covered/key_level_metadata.h"
#include "cache/covered/group_level_metadata.h"
#include "common/key.h"
#include "common/value.h"

namespace covered
{
    typedef std::list<std::pair<Key, KeyLevelMetadata>> perkey_metadata_list_t; // LRU list of object-level metadata
    typedef std::unordered_map<GroupId, GroupLevelMetadata> pergroup_metadata_map_t;

    // NOTE: typedef MUST need complete definition of class unless you use pointers or references -> cannot use perkey_lookup_iter_t in sorted_popularity_multimap_t, which will cause circular dependency between LookupMetadata and sorted_popularity_multimap_t (using Key here is just for implementation simplicity, yet actually we can move CacheItem pointers from MMContainer's LRU list into popularity list to replace keys for popularity list if we hack cachelib)
    //typedef std::multimap<Popularity, LruCacheReadHandle> sorted_popularity_multimap_t; // Obselete: local uncached objects cannot provide LruCacheReadHandle
    typedef std::multimap<Popularity, Key> sorted_popularity_multimap_t; // Ordered list of per-key popularity

    class LookupMetadata
    {
    public:
        LookupMetadata();
        LookupMetadata(const LookupMetadata& other);
        ~LookupMetadata();

        perkey_metadata_list_t::iterator getPerkeyMetadataIter() const;
        sorted_popularity_multimap_t::iterator getSortedPopularityIter() const;

        void setPerkeyMetadataIter(const perkey_metadata_list_t::iterator& perkey_metadata_iter);
        void setSortedPopularityIter(const sorted_popularity_multimap_t::iterator& sorted_popularity_iter);

        static uint64_t getPerkeyMetadataIterSizeForCapacity();
        static uint64_t getSortedPopularityIterSizeForCapacity();

        const LookupMetadata& operator=(const LookupMetadata& other);
    private:
        static const std::string kClassName;

        perkey_metadata_list_t::iterator perkey_metadata_iter_;
        sorted_popularity_multimap_t::iterator sorted_popularity_iter_;
    };

    // NOTE: we MUST store Key in ordered list to locate lookup table during eviciton; use duplicate Keys in lookup table to update ordered list -> if we store Key pointer in ordered list and use duplicate popularity/LRU-order in lookup table, we still can locate lookup table during eviction, yet cannot locate the corresponding popularity entry / have to access all LRU entries to update ordered list
    typedef std::unordered_map<Key, LookupMetadata, KeyHasher> perkey_lookup_table_t;
    typedef perkey_lookup_table_t::iterator perkey_lookup_iter_t;
    typedef perkey_lookup_table_t::const_iterator perkey_lookup_const_iter_t;

    class CacheMetadataBase
    {
    public:
        CacheMetadataBase();
        virtual ~CacheMetadataBase();

        // Common functions

        bool isKeyExist(const Key& key) const; // Check if key has been admitted or tracked for local cached or uncached object
        bool getLeastPopularKey(const uint32_t& least_popular_rank, Key& key) const; // Get ith least popular key for local cached or uncached object

        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove admitted cached key or tracked uncached key (for getrsp with cache miss, put/delrsp with cache miss, admission, eviction)

        virtual uint64_t getSizeForCapacity() const = 0; // Get size for capacity constraint (different for local cached or uncached objects)
    private:
        static const std::string kClassName;
    protected:
        // Common functions

        void addForNewKey_(const Key& key, const Value& value); // Newly admitted cached key or currently tracked uncached key (for admission, getrsp with cache miss, put/delrsp with cache miss)

        void updateForExistingKey_(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related); // Admitted cached key (is_value_related = false: for getreq with cache hit; is_value_related = true: for getrsp with invalid hit, put/delreq with cache hit); Or tracked uncached key (is_value_related = false: for getrsp with cache miss; is_value_related = true: put/delrsp with cache miss)

        // For object-level metadata
        perkey_metadata_list_t::iterator addPerkeyMetadata_(const Key& key, const GroupId& assigned_group_id); // Return new perkey metadata iterator
        const KeyLevelMetadata& updatePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter); // Return updated KeyLevelMetadata
        void removePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter);

        // For group-level metadata
        const GroupLevelMetadata& getGroupLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const; // Return existing GroupLevelMetadata
        const GroupLevelMetadata& addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id); // Return added/updated GroupLevelMetadata
        const GroupLevelMetadata& updatePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value, const bool& is_value_related); // Return updated GroupLevelMetadata
        void removePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value);

        // For popularity information
        Popularity getPopularity_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const;
        uint32_t getLeastPopularRank_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const;
        Popularity calculatePopularity_(const KeyLevelMetadata& key_level_statistics, const GroupLevelMetadata& group_level_statistics) const; // Calculate popularity based on object-level and group-level metadata
        sorted_popularity_multimap_t::iterator addPopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter); // Return new sorted popularity iterator
        sorted_popularity_multimap_t::iterator updatePopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter); // Return updated sorted popularity iterator
        void removePopularity_(const perkey_lookup_iter_t& perkey_lookup_iter);

        // For lookup table
        perkey_lookup_iter_t getLookup_(const Key& key); // Return lookup iterator (assert result != end())
        perkey_lookup_const_iter_t getLookup_(const Key& key) const; // Return lookup const iterator (assert result != end())
        perkey_lookup_iter_t tryToGetLookup_(const Key& key); // Return lookup iterator (end() if not found)
        perkey_lookup_const_iter_t tryToGetLookup_(const Key& key) const; // Return lookup iterator (end() if not found)
        perkey_lookup_iter_t addLookup_(const Key& key); // Return new lookup iterator
        void updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const sorted_popularity_multimap_t::iterator& new_sorted_popularity_iter);
        void updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const perkey_metadata_list_t::iterator& perkey_metadata_iter, const sorted_popularity_multimap_t::iterator& sorted_popularity_iter);
        void removeLookup_(const perkey_lookup_iter_t& perkey_lookup_iter);
        
        // Object-level metadata
        uint64_t perkey_metadata_list_key_size_; // Total size of keys in perkey_metadata_list_
        perkey_metadata_list_t perkey_metadata_list_; // LRU list for object-level metadata (list index is recency information)

        // Group-level metadata
        GroupId cur_group_id_;
        uint32_t cur_group_keycnt_;
        pergroup_metadata_map_t pergroup_metadata_map_; // Group-level metadata (grouping based on admission/tracked time)

        // TODO: For local cached metadata, use local reward instead of local/redirected cached popularity as the ordered list
        // TODO: For local uncached metadata, use approximate admission benefit instead of local uncached popularity as the ordered list (but we still piggyback local uncached popularity for popularity collection)

        // Popularity information
        // OBSOLETE (learned index cannot support duplicate popularities; actually we do NOT count the pointers of std::multimap in cache size usage): Use learned index to replace local cached/uncached sorted_popularity_ for less memory usage (especially for local cached objects due to limited # of uncached objects)
        uint64_t sorted_popularity_multimap_key_size_; // Total size of keys in sorted_popularity_multimap_
        sorted_popularity_multimap_t sorted_popularity_multimap_; // Sorted popularity information (ascending order; allow duplicate popularity values)

        // Lookup table
        uint64_t perkey_lookup_table_key_size_; // Total size of keys in perkey_lookup_table_
        perkey_lookup_table_t perkey_lookup_table_;
    };
}

#endif