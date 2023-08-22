#include "cache/covered/sorted_popularity_multimap.h"

namespace covered
{
    const std::string SortedPopularityMultimap::kClassName("SortedPopularityMultimap");

    SortedPopularityMultimap::SortedPopularityMultimap()
    {
        sorted_popularity_multimap_.clear();
        popularity_lookup_table_.clear();
    }
    
    SortedPopularityMultimap::~SortedPopularityMultimap() {}

    bool SortedPopularityMultimap::isKeyExist(const Key& key) const
    {
        popularity_lookup_table_t::const_iterator lookup_iter = popularity_lookup_table_.find(key);
        bool is_key_exist = (lookup_iter != popularity_lookup_table_.end());

        return is_key_exist;
    }

    void SortedPopularityMultimap::updateForExistingKey(const Key& key, const KeyLevelStatistics& key_level_statistics, const GroupLevelStatistics& group_level_statistics)
    {
        // Calculate popularity
        Popularity new_popularity = calculatePopularity_(key_level_statistics, group_level_statistics);

        // Check lookup iterator and popularity iterator
        popularity_lookup_table_t::iterator lookup_iter = popularity_lookup_table_.find(key);
        assert(lookup_iter != popularity_lookup_table_.end());
        multimap_iterator_t old_popularity_iter = lookup_iter->second;
        assert(old_popularity_iter != sorted_popularity_multimap_.end());
        assert(old_popularity_iter->second == lookup_iter);

        /*// Get handle referring to item by move assignment operator
        // NOTE: now handle points to item, yet old_popularity_iter->second points to NULL)
        LruCacheReadHandle handle = std::move(old_popularity_iter->second);*/

        // Remove old popularity information
        sorted_popularity_multimap_.erase(old_popularity_iter);
        
        /*// Create pair for new popularity by std::pair<T1, T2>(T1&&, T2&&) -> move constructor of T1 and T2
        // NOTE: now tmp_pair->second points to item, yet handle points to NULL
        std::pair<Popularity, LruCacheReadHandle> tmp_pair(std::move(new_popularity), std::move(handle));*/

        /*// Add new popularity information by std::multimap::insert(value_type&&) -> move constructor of std::pair<T1, T2>(std::pair&&) -> move constructor of T1 and T2 (note that members of rvalue is still rvalue)
        // NOTE: now new_popularity_iter points to item, yet tmp_pair->second points to NULL
        multimap_iterator_t new_popularity_iter = sorted_popularity_multimap_.insert(std::move(tmp_pair));*/

        // Add new popularity information
        multimap_iterator_t new_popularity_iter = sorted_popularity_multimap_.insert(std::pair(new_popularity, lookup_iter));

        // Update lookup table
        lookup_iter->second = new_popularity_iter;

        return;
    }

    Popularity SortedPopularityMultimap::calculatePopularity_(const KeyLevelStatistics& perkey_statistics, const GroupLevelStatistics& pergroup_statistics) const
    {
        // TODO: Use heuristic or learned approach to calculate popularity (refer to state-of-the-art studies such as LRB and GL-Cache)

        // NOTE: Here we use a simple approach to calculate popularity based on object-level and group-level statistics
        Popularity popularity = static_cast<Popularity>(pergroup_statistics.getAvgObjectSize()) / static_cast<Popularity>(perkey_statistics.getFrequency()); // # of accessed bytes per cache access (similar as LHD)
        return popularity;
    }
}