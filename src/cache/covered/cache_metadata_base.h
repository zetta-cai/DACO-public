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

        virtual uint64_t getSizeForCapacity() const = 0; // Get size for capacity constraint (different for local cached or uncached objects)
    private:
        static const std::string kClassName;
    protected:
        // Common functions

        void addForNewKey_(const Key& key, const Value& value); // For admission and getrsp/put/delreq w/ miss (also getreq w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), intialize and update object-/group-level metadata (both value-unrelated and value-related) for newly admitted cached key or currently tracked uncached key

        void updateNoValueStatsForExistingKey_(const Key& key); // For get/put/delreq w/ hit/miss, update object-/group-level value-unrelated metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached)
        void updateValueStatsForExistingKey_(const Key& key, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit (also getrsp w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), update object-/group-level value-related metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached)

        void removeForExistingKey_(const Key& detracked_key, const Value& value, const bool& is_local_cached_metadata); // Remove admitted cached key or tracked uncached key (for getrsp with cache miss, put/delrsp with cache miss, admission, eviction)

        ObjectSize getObjectSize_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const; // Get accurate/approximate object size

        // For object-level metadata
        const KeyLevelMetadata& getkeyLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const; // Return existing KeyLevelMetadata
        perkey_metadata_list_t::iterator addPerkeyMetadata_(const Key& key, const Value& value, const GroupId& assigned_group_id); // For admission and getrsp/put/delreq w/ miss (also getreq w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), initialize and update key-level value-unrelated and value-related metadata for newly-admited/tracked key; return new perkey metadata iterator
        const KeyLevelMetadata& updateNoValuePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter); // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached); return updated KeyLevelMetadata
        const KeyLevelMetadata& updateValuePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit (also getrsp w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), update object-level value-related metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached); return updated KeyLevelMetadata
        void removePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter);

        // For group-level metadata
        const GroupLevelMetadata& getGroupLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const; // Return existing GroupLevelMetadata
        const GroupLevelMetadata& addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id); // For admission and getrsp/put/delreq w/ miss (also getreq w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), initialize and update group-level value-unrelated and value-related metadata for newly-admited/tracked key; return added/updated GroupLevelMetadata
        const GroupLevelMetadata& updateNoValuePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter); // For get/put/delreq w/ hit/miss, update group-level value-unrelated metadata for the key already in the group (i.e., already admitted/tracked objects for local cached/uncached); return updated GroupLevelMetadata
        const GroupLevelMetadata& updateValuePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit (also getrsp w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP), update group-level value-related metadata for the key already in the group (i.e., already admitted/tracked objects for local cached/uncached); return updated GroupLevelMetadata
        void removePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const bool& is_local_cached_metadata);

        // For popularity information
        Popularity getPopularity_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const;
        uint32_t getLeastPopularRank_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const;
        Popularity calculatePopularity_(const KeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref) const; // Calculate popularity based on object-level and group-level metadata
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