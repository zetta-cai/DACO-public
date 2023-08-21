/*
 * SortedPopularityMultimap: a multimap (i.e., sorted linked list) to track mappings from key into popularity.
 * 
 * By Siyuan Sheng (2023.08.21).
 */

#ifndef SORTED_POPULARITY_MULTIMAP_H
#define SORTED_POPULARITY_MULTIMAP_H

#include <map> // std::multimap
#include <string>
#include <unordered_map> // std::unordered_map

#include "cache/covered/common_header.h"

// NOTE: we do NOT count the total size of popularity_lookup_table_ here, as per-key popularity iterator can be maintained in cachelib::ChainedHashTable for local cached objects (we track it here individually just for implementation simplicity) -> getSizeForCapacityWithoutLookupTable
// NOTE: we have to count the total size of popularity_lookup_table_ for local uncached objects -> getSizeForCapacityWithLookupTable

namespace covered
{
    class SortedPopularityMultimap
    {
    public:
        SortedPopularityMultimap();
        ~SortedPopularityMultimap();

        void updateForExistingKey(const Key& key, const KeyLevelStatistics& key_level_statistics, const GroupLevelStatistics& group_level_statistics); // Admitted cached key or tracked uncached key

        uint64_t getSizeForCapacity() const;
    private:
        //typedef std::multimap<Popularity, LruCacheReadHandle> sorted_popularity_multimap_t; // Obselete: local uncached objects cannot provide LruCacheReadHandle
        class lookup_table_iterator_t; // Forward declaration
        typedef std::multimap<Popularity, lookup_table_iterator_t> sorted_popularity_multimap_t;
        typedef sorted_popularity_multimap_t::iterator multimap_iterator_t;
        typedef std::unordered_map<Key, multimap_iterator_t, KeyHasher> popularity_lookup_table_t;
        typedef popularity_lookup_table_t::iterator lookup_table_iterator_t;

        static const std::string kClassName;

        // Popularity calculation
        Popularity calculatePopularity_(const KeyLevelStatistics& key_level_statistics, const GroupLevelStatistics& group_level_statistics) const; // Calculate popularity based on object-level and group-level statistics

        sorted_popularity_multimap_t sorted_popularity_multimap_;
        popularity_lookup_table_t popularity_lookup_table_;
    };
}

#endif