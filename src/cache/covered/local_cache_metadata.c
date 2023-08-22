#include "cache/covered/local_cache_metadata.h"

namespace covered
{
    // LookupMetadata

    const std::string LookupMetadata::kClassName("LookupMetadata");

    LookupMetadata::LookupMetadata() {}

    LookupMetadata::~LookupMetadata() {}

    perkey_metadata_list_t::iterator LookupMetadata::getPerkeyMetadataIter() const
    {
        return perkey_metadata_iter_;
    }

    sorted_popularity_multimap_t::iterator LookupMetadata::getSortedPopularityIter() const
    {
        return sorted_popularity_iter_;
    }

    void LookupMetadata::setPerkeyMetadataIter(const perkey_metadata_list_t::iterator& perkey_metadata_iter)
    {
        perkey_metadata_iter_ = perkey_metadata_iter;
        return;
    }
    
    void LookupMetadata::setSortedPopularityIter(const sorted_popularity_multimap_t::iterator& sorted_popularity_iter)
    {
        sorted_popularity_iter_ = sorted_popularity_iter;
        return;
    }

    // LocalCacheMetadata

    const std::string LocalCacheMetadata::kClassName("LocalCacheMetadata");

    LocalCacheMetadata::LocalCacheMetadata()
    {
        perkey_metadata_list_.clear();

        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_metadata_map_.clear();

        sorted_popularity_multimap_.clear();

        perkey_lookup_table_.clear();
    }
    
    LocalCacheMetadata::~LocalCacheMetadata() {}

    bool LocalCacheMetadata::isKeyExist(const Key& key) const
    {
        perkey_lookup_table_t::const_iterator perkey_lookup_iter = perkey_lookup_table_.find(key);
        return (perkey_lookup_iter != perkey_lookup_table_.end());
    }

    void LocalCacheMetadata::addForNewKey(const Key& key, const Value& value)
    {
        // TODO: Add lookup iterator for new key

        // TODO: Add object-level metadata for new key

        // Add group-level metadata for new key
        GroupId group_id = assignGroupIdForNewKey_();
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.insert(std::pair(group_id, GroupLevelMetadata())).first;
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        pergroup_metadata_iter->second.updateForNewlyGrouped(key, value);

        // TODO: Add popularity for new key

        return;
    }

    void LocalCacheMetadata::updateForExistingKey(const Key& key)
    {
        // Get lookup iterator
        perkey_lookup_table_t::iterator perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;

        // Update object-level metadata
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end());
        perkey_metadata_iter->second.updateDynamicMetadata();

        // TODO: Update LRU list order

        // Update group-level metadata
        GroupId tmp_group_id = perkey_metadata_iter->second.getGroupId(); // Get group ID 
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        pergroup_metadata_iter->second.updateForInGroupKey(key);

        // Update popularity
        sorted_popularity_multimap_t::iterator old_sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(old_sorted_popularity_iter != sorted_popularity_multimap_.end());
        Popularity new_popularity = calculatePopularity_(perkey_metadata_iter->second, pergroup_metadata_iter->second); // Calculate popularity
        updatePopularity_(old_sorted_popularity_iter, new_popularity, perkey_lookup_iter);

        return;
    }

    GroupId LocalCacheMetadata::assignGroupIdForNewKey_()
    {
        cur_group_keycnt_++;
        if (cur_group_keycnt_ > COVERED_PERGROUP_MAXKEYCNT)
        {
            cur_group_id_++;
            cur_group_keycnt_ = 1;
        }

        return cur_group_id_;
    }

    void LocalCacheMetadata::updatePopularity_(const sorted_popularity_multimap_t::iterator& old_sorted_popularity_iter, const Popularity& new_popularity, perkey_lookup_iter_t& perkey_lookup_iter)
    {
        assert(old_sorted_popularity_iter->second == perkey_lookup_iter);

        /*// Get handle referring to item by move assignment operator
        // NOTE: now handle points to item, yet old_sorted_popularity_iter->second points to NULL)
        LruCacheReadHandle handle = std::move(old_sorted_popularity_iter->second);*/

        // Remove old popularity information
        sorted_popularity_multimap_.erase(old_sorted_popularity_iter);
        
        /*// Create pair for new popularity by std::pair<T1, T2>(T1&&, T2&&) -> move constructor of T1 and T2
        // NOTE: now tmp_pair->second points to item, yet handle points to NULL
        std::pair<Popularity, LruCacheReadHandle> tmp_pair(std::move(new_popularity), std::move(handle));*/

        /*// Add new popularity information by std::multimap::insert(value_type&&) -> move constructor of std::pair<T1, T2>(std::pair&&) -> move constructor of T1 and T2 (note that members of rvalue is still rvalue)
        // NOTE: now new_popularity_iter points to item, yet tmp_pair->second points to NULL
        multimap_iterator_t new_popularity_iter = sorted_popularity_multimap_.insert(std::move(tmp_pair));*/

        // Add new popularity information
        sorted_popularity_multimap_t::iterator new_popularity_iter = sorted_popularity_multimap_.insert(std::pair(new_popularity, perkey_lookup_iter));

        // Update lookup table
        perkey_lookup_iter->second.setSortedPopularityIter(new_popularity_iter);

        return;
    }

    Popularity LocalCacheMetadata::calculatePopularity_(const KeyLevelMetadata& perkey_statistics, const GroupLevelMetadata& pergroup_statistics) const
    {
        // TODO: Use heuristic or learned approach to calculate popularity (refer to state-of-the-art studies such as LRB and GL-Cache)

        // NOTE: Here we use a simple approach to calculate popularity based on object-level and group-level metadata
        Popularity popularity = static_cast<Popularity>(pergroup_statistics.getAvgObjectSize()) / static_cast<Popularity>(perkey_statistics.getFrequency()); // # of accessed bytes per cache access (similar as LHD)
        return popularity;
    }
}