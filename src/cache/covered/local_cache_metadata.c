#include "cache/covered/local_cache_metadata.h"

#include <assert.h>

namespace covered
{
    // LookupMetadata

    const std::string LookupMetadata::kClassName("LookupMetadata");

    LookupMetadata::LookupMetadata() {}

    LookupMetadata::LookupMetadata(const LookupMetadata& other)
    {
        perkey_metadata_iter_ = other.perkey_metadata_iter_;
        sorted_popularity_iter_ = other.sorted_popularity_iter_;
    }

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

    LocalCacheMetadata::LocalCacheMetadata(const bool& is_for_uncached_objects) : is_for_uncached_objects_(is_for_uncached_objects)
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

    bool LocalCacheMetadata::needDetrackForUncachedObjects(Key& detracked_key) const
    {
        // NOTE: we only limit the number of tracked metadata for local uncached objects
        assert(is_for_uncached_objects_);
        
        uint32_t cur_trackcnt = perkey_lookup_table_.size();
        if (cur_trackcnt > COVERED_LOCAL_UNCACHED_MAX_TRACKCNT)
        {
            detracked_key = sorted_popularity_multimap_.begin()->second->first;
            return true;
        }
        else
        {
            return false;
        }
    }

    uint32_t LocalCacheMetadata::getApproxDetrackValueForUncachedObjects(const Key& detracked_key) const
    {
        // NOTE: we only get approximate detrack value size for local uncached objects
        assert(is_for_uncached_objects_);

        // Get lookup iterator
        perkey_lookup_const_iter_t perkey_lookup_const_iter = getLookup_(detracked_key);

        // Get average object size
        const GroupLevelMetadata& pergroup_metadata_ref = getGroupLevelMetadata_(perkey_lookup_const_iter);
        uint32_t avg_object_size = pergroup_metadata_ref.getAvgObjectSize();

        // NOTE: for local uncached objects, as we do NOT know per-key value size, we use the (average object size - key size) as the approximated detrack value
        uint32_t approx_detrack_value_size = 0;
        uint32_t detrack_key_size = detracked_key.getKeystr().length();
        if (avg_object_size > detrack_key_size)
        {
            approx_detrack_value_size = avg_object_size - detrack_key_size;
        }

        return approx_detrack_value_size;
    }

    void LocalCacheMetadata::addForNewKey(const Key& key, const Value& value)
    {
        // Add lookup iterator for new key
        perkey_lookup_iter_t perkey_lookup_iter = addLookup_(key);

        // Add group-level metadata for new key
        GroupId assigned_group_id = 0;
        const GroupLevelMetadata& pergroup_metadata_ref = addPergroupMetadata_(key, value, assigned_group_id);

        // Add object-level metadata for new key
        perkey_metadata_list_t::iterator perkey_metadata_iter = addPerkeyMetadata_(key, assigned_group_id);
        const KeyLevelMetadata& perkey_metadata_ref = perkey_metadata_iter->second;

        // Add popularity for new key
        Popularity new_popularity = calculatePopularity_(perkey_metadata_ref, pergroup_metadata_ref); // Calculate popularity
        sorted_popularity_multimap_t::iterator sorted_popularity_iter = addPopularity_(new_popularity, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, perkey_metadata_iter, sorted_popularity_iter);

        return;
    }

    void LocalCacheMetadata::updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related)
    {
        // Get lookup iterator
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(key);

        // Update object-level metadata
        const KeyLevelMetadata& perkey_metadata_ref = updatePerkeyMetadata_(perkey_lookup_iter);

        // Update group-level metadata
        const GroupLevelMetadata& pergroup_metadata_ref = updatePergroupMetadata_(perkey_lookup_iter, key, value, original_value, is_value_related);

        // Update popularity
        Popularity new_popularity = calculatePopularity_(perkey_metadata_ref, pergroup_metadata_ref); // Calculate popularity
        sorted_popularity_multimap_t::iterator new_sorted_popularity_iter = updatePopularity_(new_popularity, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, new_sorted_popularity_iter);

        return;
    }

    void LocalCacheMetadata::removeForExistingKey(const Key& detracked_key, const Value& value)
    {
        // Get lookup iterator
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(detracked_key);

        // Remove group-level metadata
        removePergroupMetadata_(perkey_lookup_iter, detracked_key, value);

        // Remove object-level metadata
        removePerkeyMetadata_(perkey_lookup_iter);
        perkey_lookup_iter->second.setPerkeyMetadataIter(perkey_metadata_list_.end());

        // Remove popularity
        removePopularity_(perkey_lookup_iter);
        perkey_lookup_iter->second.setSortedPopularityIter(sorted_popularity_multimap_.end());

        // Remove lookup table
        removeLookup_(perkey_lookup_iter);

        return;
    }

    // For object-level metadata

    perkey_metadata_list_t::iterator LocalCacheMetadata::addPerkeyMetadata_(const Key& key, const GroupId& assigned_group_id)
    {
        // NOTE: NO need to verify key existence due to LRU-based list

        // Add object-level metadata for new key
        perkey_metadata_list_.push_front(std::pair<Key, KeyLevelMetadata>(key, KeyLevelMetadata(assigned_group_id)));
        perkey_metadata_list_t::iterator perkey_metadata_iter = perkey_metadata_list_.begin();
        assert(perkey_metadata_iter != perkey_metadata_list_.end());

        perkey_metadata_iter->second.updateDynamicMetadata();

        // TODO: Update LRU list order

        return perkey_metadata_iter;
    }
    
    const KeyLevelMetadata& LocalCacheMetadata::updatePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        perkey_metadata_iter->second.updateDynamicMetadata();

        // TODO: Update LRU list order

        return perkey_metadata_iter->second;
    }

    void LocalCacheMetadata::removePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        perkey_metadata_list_.erase(perkey_metadata_iter);

        // NOTE: NO need to update LRU list order

        return;
    }

    // For group-level metadata

    const GroupLevelMetadata& LocalCacheMetadata::getGroupLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_const_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        GroupId tmp_group_id = perkey_metadata_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::const_iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        return pergroup_metadata_iter->second;
    }

    const GroupLevelMetadata& LocalCacheMetadata::addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id)
    {
        // NOTE: NO need to verify key existence due to duplicate group IDs

        // Assigne group ID for new key
        cur_group_keycnt_++;
        if (cur_group_keycnt_ > COVERED_PERGROUP_MAXKEYCNT)
        {
            cur_group_id_++;
            cur_group_keycnt_ = 1;
        }
        assigned_group_id = cur_group_id_;

        // Add group-level metadata for new key
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(assigned_group_id);
        if (pergroup_metadata_iter == pergroup_metadata_map_.end())
        {
            pergroup_metadata_iter = pergroup_metadata_map_.insert(std::pair(assigned_group_id, GroupLevelMetadata())).first;
        }
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        pergroup_metadata_iter->second.updateForNewlyGrouped(key, value);

        return pergroup_metadata_iter->second;
    }

    const GroupLevelMetadata& LocalCacheMetadata::updatePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value, const bool& is_value_related)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        GroupId tmp_group_id = perkey_metadata_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        if (!is_value_related) // Unrelated with value
        {
            pergroup_metadata_iter->second.updateForInGroupKey(key);
        }
        else // Related with value
        {
            pergroup_metadata_iter->second.updateForInGroupKeyValue(key, value, original_value);
        }

        return pergroup_metadata_iter->second;
    }

    void LocalCacheMetadata::removePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        GroupId tmp_group_id = perkey_metadata_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        bool is_group_empty = pergroup_metadata_iter->second.updateForDegrouped(key, value);
        if (is_group_empty)
        {
            pergroup_metadata_map_.erase(pergroup_metadata_iter);
        }

        return;
    }

    // For popularity information

    Popularity LocalCacheMetadata::calculatePopularity_(const KeyLevelMetadata& perkey_statistics, const GroupLevelMetadata& pergroup_statistics) const
    {
        // TODO: Use heuristic or learned approach to calculate popularity (refer to state-of-the-art studies such as LRB and GL-Cache)

        // NOTE: Here we use a simple approach to calculate popularity based on object-level and group-level metadata
        Popularity popularity = static_cast<Popularity>(pergroup_statistics.getAvgObjectSize()) / static_cast<Popularity>(perkey_statistics.getFrequency()); // # of accessed bytes per cache access (similar as LHD)
        return popularity;
    }

    sorted_popularity_multimap_t::iterator LocalCacheMetadata::addPopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // NOTE: NO need to verify key existence due to duplicate popularity values
        // NOTE: perkey_metadata_iter_ and sorted_popularity_iter_ in perkey_lookup_iter here are invalid

        // Add new popularity information
        sorted_popularity_multimap_t::iterator new_popularity_iter = sorted_popularity_multimap_.insert(std::pair(new_popularity, perkey_lookup_iter));

        return new_popularity_iter;
    }

    sorted_popularity_multimap_t::iterator LocalCacheMetadata::updatePopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::iterator old_sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(old_sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key
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

        return new_popularity_iter;
    }

    void LocalCacheMetadata::removePopularity_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::iterator sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key
        assert(sorted_popularity_iter->second == perkey_lookup_iter);

        // Remove popularity information
        sorted_popularity_multimap_.erase(sorted_popularity_iter);

        return;
    }

    // For lookup table

    perkey_lookup_iter_t LocalCacheMetadata::getLookup_(const Key& key)
    {
        perkey_lookup_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    perkey_lookup_const_iter_t LocalCacheMetadata::getLookup_(const Key& key) const
    {
        perkey_lookup_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    perkey_lookup_iter_t LocalCacheMetadata::addLookup_(const Key& key)
    {
        // Verify that key must NOT exist (NOT admitted/tracked)
        perkey_lookup_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter == perkey_lookup_table_.end());

        // Add new lookup metadata
        perkey_lookup_iter = perkey_lookup_table_.insert(std::pair(key, LookupMetadata())).first;
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    void LocalCacheMetadata::updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const sorted_popularity_multimap_t::iterator& new_sorted_popularity_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setSortedPopularityIter(new_sorted_popularity_iter);

        return;
    }

    void LocalCacheMetadata::updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const perkey_metadata_list_t::iterator& perkey_metadata_iter, const sorted_popularity_multimap_t::iterator& sorted_popularity_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setPerkeyMetadataIter(perkey_metadata_iter);
        perkey_lookup_iter->second.setSortedPopularityIter(sorted_popularity_iter);

        return;
    }

    void LocalCacheMetadata::removeLookup_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_table_.erase(perkey_lookup_iter);

        return;
    }
}