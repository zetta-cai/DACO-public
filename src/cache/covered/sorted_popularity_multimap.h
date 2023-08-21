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

// NOTE: we do NOT count the size of keys here, as per-key popularity iterator can be maintained in cachelib::CacheItem for local cached objects or per-key statistics for local uncached objects, we track them individually just for implementation simplicity

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
        typedef std::multimap<Popularity, LruCacheReadHandle> sorted_popularity_multimap_t;
        typedef sorted_popularity_multimap_t::iterator popularity_iterator_t;
        typedef std::unordered_map<Key, popularity_iterator_t, KeyHasher> popularity_lookup_table_t;

        static const std::string kClassName;

        // Popularity calculation
        Popularity calculatePopularity_(const KeyLevelStatistics& key_level_statistics, const GroupLevelStatistics& group_level_statistics) const; // Calculate popularity based on object-level and group-level statistics

        sorted_popularity_multimap_t sorted_popularity_multimap_;
        popularity_lookup_table_t popularity_lookup_table_;
    };
}

#endif