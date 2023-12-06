/*
 * CacheMetadataBase: metadata for covered local cache (including object-level, group-level and sorted popularity metadata).
 *
 * NOTE: object-level metadata in CacheMetadataBase is updated for local requests (local hits for local cached objects and local misses for local uncached objects), while group-level metadata is updated for all requests (local/redirected hits for local cached objects and local misses for local uncached objects).
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

#include "cache/covered/group_level_metadata.h"
#include "cache/covered/reward_compare.h"
#include "common/covered_common_header.h"
#include "common/key.h"
#include "common/value.h"
#include "edge/edge_wrapper.h"

namespace covered
{
    // NOTE: typedef MUST need complete definition of class unless you use pointers or references -> cannot use perkey_lookup_table_t::iterator in sorted_reward_multimap_t, which will cause circular dependency between LookupMetadata and sorted_reward_multimap_t (using Key here is just for implementation simplicity, yet actually we can hack TommyDS to organize key-value pairs as reward-based ordered list)
    //typedef std::multimap<Reward, LruCacheReadHandle> sorted_reward_multimap_t; // Obselete: local uncached objects cannot provide LruCacheReadHandle

    // NOTE: using which policy for equal-reward objects has little difference, the key point is that we should NOT simply set reward as zero for one-hit-wonders, as we may mis-evict some hot keys whose frequency is 1 during cache warmup
    typedef std::multimap<Reward, Key, RewardLruCompare> sorted_reward_multimap_t; // Ordered list of per-key reward (with LRU policy for one-hit-wonders)
    //typedef std::multimap<Reward, Key, RewardMruCompare> sorted_reward_multimap_t; // Ordered list of per-key reward (with MRU policy for one-hit-wonders)

    template<class T>
    class LookupMetadata
    {
    public:
        typedef typename T::iterator perkey_metadata_list_iter_t;

        LookupMetadata();
        LookupMetadata(const LookupMetadata& other);
        ~LookupMetadata();

        perkey_metadata_list_iter_t getPerkeyMetadataListIter() const;
        sorted_reward_multimap_t::iterator getSortedRewardIter() const;

        void setPerkeyMetadataListIter(const perkey_metadata_list_iter_t& perkey_metadata_list_iter);
        void setSortedRewardIter(const sorted_reward_multimap_t::iterator& sorted_reward_iter);

        static uint64_t getPerkeyMetadataListIterSizeForCapacity();
        static uint64_t getSortedRewardIterSizeForCapacity();

        const LookupMetadata& operator=(const LookupMetadata& other);
    private:
        static const std::string kClassName;

        perkey_metadata_list_iter_t perkey_metadata_list_iter_; // Key-level metadata for local requests
        sorted_reward_multimap_t::iterator sorted_reward_iter_;
    };

    template<class T>
    class CacheMetadataBase
    {
    public:
        typedef std::list<std::pair<Key, T>> perkey_metadata_list_t; // LRU list of homogeneous object-level metadata
        typedef typename perkey_metadata_list_t::iterator perkey_metadata_list_iter_t;
        typedef std::unordered_map<GroupId, GroupLevelMetadata> pergroup_metadata_map_t;

        // NOTE: we MUST store Key in ordered list to locate lookup table during eviciton; use duplicate Keys in lookup table to update ordered list -> if we store Key pointer in ordered list and use duplicate popularity/LRU-order in lookup table, we still can locate lookup table during eviction, yet cannot locate the corresponding popularity entry / have to access all LRU entries to update ordered list
        typedef std::unordered_map<Key, LookupMetadata<perkey_metadata_list_t>, KeyHasher> perkey_lookup_table_t;
        typedef typename perkey_lookup_table_t::iterator perkey_lookup_table_iter_t;
        typedef typename perkey_lookup_table_t::const_iterator perkey_lookup_table_const_iter_t;

        CacheMetadataBase();
        virtual ~CacheMetadataBase();

        bool isKeyExist(const Key& key) const; // Check if key has been admitted or tracked for local cached or uncached object

        // For newly-admited/tracked keys
        // NOTE: for admission and getrsp/put/delreq w/ miss, intialize and update object-/group-level metadata (both value-unrelated and value-related) for newly admitted cached key or currently tracked uncached key
        bool addForNewKey(const EdgeWrapper* edge_wrapper_ptr, const Key& key, const Value& value, const uint32_t& peredge_synced_victimcnt, const bool& is_global_cached, const bool& is_neighbor_cached); // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)

        // For existing key
        // NOTE: for get/put/delreq w/ hit/miss, update object-/group-level value-unrelated metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached)
        bool updateNoValueStatsForExistingKey(const EdgeWrapper* edge_wrapper_ptr, const Key& key, const uint32_t& peredge_synced_victimcnt, const bool& is_redirected, const bool& is_global_cached); // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)
        // NOTE: for put/delreq w/ hit/miss and getrsp w/ invalid-hit, update object-/group-level value-related metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached)
        bool updateValueStatsForExistingKey(const EdgeWrapper* edge_wrapper_ptr, const Key& key, const Value& value, const Value& original_value, const uint32_t& peredge_synced_victimcnt); // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)
        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove admitted cached key or tracked uncached key (for getrsp with cache miss, put/delrsp with cache miss, admission, eviction)

        // For object size
        ObjectSize getObjectSize(const Key& key) const; // Get accurate/approximate object size

        // For popularity information
        virtual void getPopularity(const Key& key, Popularity& local_uncached_popularity, Popularity& redirected_popularity) const = 0; // Local uncached metadata NOT set redirected_popularity

        // For reward information
        Reward getLocalRewardForExistingKey(const Key& key) const; // Get local reward for local cached or uncached object
        bool getLeastRewardKeyAndReward(const uint32_t& least_reward_rank, Key& key, Reward& reward) const; // Get ith least popular key for local cached or uncached object

        virtual uint64_t getSizeForCapacity() const = 0; // Get size for capacity constraint (different for local cached or uncached objects)
        uint64_t getSizeForCapacity_(const bool& is_local_cached_metadata) const;
    protected:
        // For object-level metadata
        const T& getkeyLevelMetadata(const Key& key) const; // Return existing key-level metadata

        // For popularity information
        virtual void calculateAndUpdatePopularity_(perkey_metadata_list_iter_t& perkey_metadata_list_iter, const T& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref) = 0;
        Popularity calculatePopularity_(const Frequency& frequency, const ObjectSize& object_size) const;

        // For reward information
        uint32_t getLeastRewardRank_(const perkey_lookup_table_const_iter_t& perkey_lookup_iter) const;
        sorted_reward_multimap_t::iterator updateReward_(const Reward& new_reward, const perkey_lookup_table_iter_t& perkey_lookup_iter); // Return updated sorted reward iterator

        // For lookup table
        perkey_lookup_table_iter_t getLookup_(const Key& key); // Return lookup iterator (assert result != end())
        perkey_lookup_table_const_iter_t getLookup_(const Key& key) const; // Return lookup const iterator (assert result != end())
        void updateLookup_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const sorted_reward_multimap_t::iterator& new_sorted_reward_iter);
        void updateLookup_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const perkey_metadata_list_iter_t& perkey_metadata_list_iter, const sorted_reward_multimap_t::iterator& sorted_reward_iter);
    private:
        static const std::string kClassName;

        // For newly-admited/tracked keys
        virtual bool afterAddForNewKey_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) = 0; // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)

        // For existing key
        virtual bool beforeUpdateStatsForExistingKey_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const = 0; // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)
        virtual bool afterUpdateStatsForExistingKey_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter, const uint32_t& peredge_synced_victimcnt) const = 0; // Return if affect local synced victims in victim tracker (always return false for local uncached metadata)

        // For object size
        ObjectSize getObjectSize_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const; // Get accurate/approximate object size

        // For object-level metadata
        const T& getkeyLevelMetadata_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const; // Return existing key-level metadata
        perkey_metadata_list_iter_t addPerkeyMetadata_(const Key& key, const Value& value, const GroupId& assigned_group_id, const bool& is_global_cached, const bool& is_neighbor_cached); // For admission and getrsp/put/delreq w/ miss, initialize and update key-level value-unrelated and value-related metadata for newly-admited/tracked key; return new perkey metadata iterator
        perkey_metadata_list_iter_t updateNoValuePerkeyMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const bool& is_redirected, const bool& is_global_cached); // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached); return updated KeyLevelMetadata
        perkey_metadata_list_iter_t updateValuePerkeyMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit, update object-level value-related metadata for existing key (i.e., already admitted/tracked objects for local cached/uncached); return updated KeyLevelMetadata
        void removePerkeyMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter);

        // For group-level metadata
        const GroupLevelMetadata& getGroupLevelMetadata_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const; // Return existing GroupLevelMetadata
        const GroupLevelMetadata& addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id); // For admission and getrsp/put/delreq w/ miss, initialize and update group-level value-unrelated and value-related metadata for newly-admited/tracked key; return added/updated GroupLevelMetadata
        const GroupLevelMetadata& updateNoValuePergroupMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter); // For get/put/delreq w/ hit/miss, update group-level value-unrelated metadata for the key already in the group (i.e., already admitted/tracked objects for local cached/uncached); return updated GroupLevelMetadata
        const GroupLevelMetadata& updateValuePergroupMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value); // For put/delreq w/ hit/miss and getrsp w/ invalid-hit, update group-level value-related metadata for the key already in the group (i.e., already admitted/tracked objects for local cached/uncached); return updated GroupLevelMetadata
        void removePergroupMetadata_(const perkey_lookup_table_iter_t& perkey_lookup_iter, const Key& key, const Value& value);

        // For reward information
        virtual Reward calculateReward_(const EdgeWrapper* edge_wrapper_ptr, perkey_metadata_list_iter_t perkey_metadata_list_iter) const = 0; // NOTE: ONLY local cached metadata needs redirected_popularity for local cached objects
        sorted_reward_multimap_t::iterator addReward_(const Reward& new_reward, const perkey_lookup_table_iter_t& perkey_lookup_iter); // Return new sorted reward iterator
        void removeReward_(const perkey_lookup_table_iter_t& perkey_lookup_iter);

        // For lookup table
        perkey_lookup_table_iter_t tryToGetLookup_(const Key& key); // Return lookup iterator (end() if not found)
        perkey_lookup_table_const_iter_t tryToGetLookup_(const Key& key) const; // Return lookup iterator (end() if not found)
        perkey_lookup_table_iter_t addLookup_(const Key& key); // Return new lookup iterator
        void removeLookup_(const perkey_lookup_table_iter_t& perkey_lookup_iter);
        
        // Object-level metadata for local hits/misses
        uint64_t perkey_metadata_list_key_size_; // Total size of keys in perkey_metadata_list_
        perkey_metadata_list_t perkey_metadata_list_; // LRU list for object-level metadata (list index is recency information (NOT used yet); descending order of recency)

        // Group-level metadata for local/redirected hits and local misses
        GroupId cur_group_id_;
        uint32_t cur_group_keycnt_;
        pergroup_metadata_map_t pergroup_metadata_map_; // Group-level metadata (grouping based on admission/tracked time)

        // Reward information
        // OBSOLETE (learned index cannot support duplicate popularities; actually we do NOT count the pointers of std::multimap in cache size usage): Use learned index to replace local cached/uncached sorted_reward_ for less memory usage (especially for local cached objects due to limited # of uncached objects)
        uint64_t sorted_reward_multimap_key_size_; // Total size of keys in sorted_reward_multimap_
        sorted_reward_multimap_t sorted_reward_multimap_; // Sorted reward information (allow duplicate reward values with insertion order; descending/ascending order for MRU/LRU of zero-reward objects)

        // Lookup table
        uint64_t perkey_lookup_table_key_size_; // Total size of keys in perkey_lookup_table_
        perkey_lookup_table_t perkey_lookup_table_;
    };
}

#endif