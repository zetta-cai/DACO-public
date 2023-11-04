#include "cache/covered/cache_metadata_base.h"

#include <assert.h>

#include "common/util.h"

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

    uint64_t LookupMetadata::getPerkeyMetadataIterSizeForCapacity()
    {
        return sizeof(perkey_metadata_list_t::iterator);
    }

    uint64_t LookupMetadata::getSortedPopularityIterSizeForCapacity()
    {
        return sizeof(sorted_popularity_multimap_t::iterator);
    }

    const LookupMetadata& LookupMetadata::operator=(const LookupMetadata& other)
    {
        perkey_metadata_iter_ = other.perkey_metadata_iter_;
        sorted_popularity_iter_ = other.sorted_popularity_iter_;

        return *this;
    }

    // CacheMetadataBase

    const std::string CacheMetadataBase::kClassName("CacheMetadataBase");

    CacheMetadataBase::CacheMetadataBase()
    {
        perkey_metadata_list_key_size_ = 0;
        perkey_metadata_list_.clear();

        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_metadata_map_.clear();

        sorted_popularity_multimap_key_size_ = 0;
        sorted_popularity_multimap_.clear();

        perkey_lookup_table_key_size_ = 0;
        perkey_lookup_table_.clear();
    }
    
    CacheMetadataBase::~CacheMetadataBase() {}

    // Common functions

    bool CacheMetadataBase::isKeyExist(const Key& key) const
    {
        perkey_lookup_table_t::const_iterator perkey_lookup_iter = perkey_lookup_table_.find(key);
        return (perkey_lookup_iter != perkey_lookup_table_.end());
    }

    bool CacheMetadataBase::getLeastPopularKey(const uint32_t& least_popular_rank, Key& key) const
    {
        bool is_least_popular_key_exist = false;

        if (least_popular_rank < sorted_popularity_multimap_.size())
        {
            sorted_popularity_multimap_t::const_iterator sorted_popularity_iter = sorted_popularity_multimap_.begin();
            std::advance(sorted_popularity_iter, least_popular_rank);
            key = sorted_popularity_iter->second;
            is_least_popular_key_exist = true;
        }

        return is_least_popular_key_exist;
    }

    void CacheMetadataBase::addForNewKey_(const Key& key, const Value& value)
    {
        // Add lookup iterator for new key
        perkey_lookup_iter_t perkey_lookup_iter = addLookup_(key);

        // Add group-level metadata for new key
        GroupId assigned_group_id = 0;
        const GroupLevelMetadata& pergroup_metadata_ref = addPergroupMetadata_(key, value, assigned_group_id);

        // Add object-level metadata for new key
        perkey_metadata_list_t::iterator perkey_metadata_iter = addPerkeyMetadata_(key, value, assigned_group_id);
        const KeyLevelMetadata& perkey_metadata_ref = perkey_metadata_iter->second;

        // Add popularity for new key
        Popularity new_popularity = calculatePopularity_(perkey_metadata_ref, pergroup_metadata_ref); // Calculate popularity
        sorted_popularity_multimap_t::iterator sorted_popularity_iter = addPopularity_(new_popularity, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, perkey_metadata_iter, sorted_popularity_iter);

        return;
    }

    void CacheMetadataBase::updateForExistingKey_(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related)
    {
        // Get lookup iterator
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(key);

        // Update object-level metadata
        const KeyLevelMetadata& perkey_metadata_ref = updatePerkeyMetadata_(perkey_lookup_iter, value, original_value, is_value_related);

        // Update group-level metadata
        const GroupLevelMetadata& pergroup_metadata_ref = updatePergroupMetadata_(perkey_lookup_iter, key, value, original_value, is_value_related);

        // Update popularity
        Popularity new_popularity = calculatePopularity_(perkey_metadata_ref, pergroup_metadata_ref); // Calculate popularity
        sorted_popularity_multimap_t::iterator new_sorted_popularity_iter = updatePopularity_(new_popularity, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, new_sorted_popularity_iter);

        return;
    }

    void CacheMetadataBase::removeForExistingKey_(const Key& detracked_key, const Value& detracked_value, const bool& is_local_cached_metadata)
    {
        // Get lookup iterator
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(detracked_key);

        // Remove group-level metadata
        removePergroupMetadata_(perkey_lookup_iter, detracked_key, detracked_value, is_local_cached_metadata);

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

    ObjectSize CacheMetadataBase::getObjectSize_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const
    {
        #ifdef TRACK_PERKEY_OBJSIZE
        const KeyLevelMetadata& perkey_metadata_ref = getkeyLevelMetadata_(perkey_lookup_const_iter);
        ObjectSize object_size = perkey_metadata_ref.getObjectSize();
        #else
        const GroupLevelMetadata& pergroup_metadata_ref = getGroupLevelMetadata_(perkey_lookup_const_iter);
        ObjectSize object_size = static_cast<ObjectSize>(pergroup_metadata_ref.getAvgObjectSize());
        #endif

        return object_size;
    }

    // For object-level metadata

    const KeyLevelMetadata& CacheMetadataBase::getkeyLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const
    {
        const LookupMetadata& lookup_metadata = perkey_lookup_const_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // NOTE: object-level metadata MUST exist for the key

        return perkey_metadata_iter->second;
    }

    perkey_metadata_list_t::iterator CacheMetadataBase::addPerkeyMetadata_(const Key& key, const Value& value, const GroupId& assigned_group_id)
    {
        // NOTE: NO need to verify key existence due to LRU-based list

        // Add object-level metadata for new key
        perkey_metadata_list_.push_front(std::pair<Key, KeyLevelMetadata>(key, KeyLevelMetadata(assigned_group_id)));
        perkey_metadata_list_t::iterator perkey_metadata_iter = perkey_metadata_list_.begin();
        assert(perkey_metadata_iter != perkey_metadata_list_.end());

        const ObjectSize object_size = key.getKeyLength() + value.getValuesize();
        perkey_metadata_iter->second.updateDynamicMetadata(object_size, 0, true);

        // push_front already places the new object-level metadata to the head of LRU list -> NO need to update LRU list order

        // Update size usage of key-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Add(perkey_metadata_list_key_size_, key.getKeyLength());

        return perkey_metadata_iter;
    }
    
    const KeyLevelMetadata& CacheMetadataBase::updatePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Value& value, const Value& original_value, const bool& is_value_related)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        // Update object-level metadata
        const ObjectSize tmp_object_size = perkey_lookup_iter->first.getKeyLength() + value.getValuesize();
        const ObjectSize tmp_original_object_size = perkey_lookup_iter->first.getKeyLength() + original_value.getValuesize();
        perkey_metadata_iter->second.updateDynamicMetadata(tmp_object_size, tmp_original_object_size, is_value_related);

        // Update LRU list order
        if (perkey_metadata_iter != perkey_metadata_list_.begin())
        {
            // Move the list entry pointed by perkey_metadata_iter to the head of the list (NOT change the memory address of the list entry)
            perkey_metadata_list_.splice(perkey_metadata_list_.begin(), perkey_metadata_list_, perkey_metadata_iter);
        }

        // NOTE: NO need to update size usage of key-level metadata

        return perkey_metadata_iter->second;
    }

    void CacheMetadataBase::removePerkeyMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        // Update size usage of key-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Minus(perkey_metadata_list_key_size_, perkey_metadata_iter->first.getKeyLength());

        perkey_metadata_list_.erase(perkey_metadata_iter);

        // NOTE: erase removes the object-level metadata yet NOT affect other iterators -> NO need to update LRU list order

        return;
    }

    // For group-level metadata

    const GroupLevelMetadata& CacheMetadataBase::getGroupLevelMetadata_(const perkey_lookup_const_iter_t& perkey_lookup_const_iter) const
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

    const GroupLevelMetadata& CacheMetadataBase::addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id)
    {
        // NOTE: NO need to verify key existence due to duplicate group IDs

        // Assigne group ID for new key
        // TODO: Use grouping settings in GL-Cache later (e.g., fix per-group cache size usage instead of COVERED_PERGROUP_MAXKEYCNT)
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
        pergroup_metadata_iter->second.updateForNewlyGrouped(key, value); // TODO: update group-level metadata will affect other keys' popularities, while we use lazy update for those popularities (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead

        return pergroup_metadata_iter->second;
    }

    const GroupLevelMetadata& CacheMetadataBase::updatePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value, const bool& is_value_related)
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
            pergroup_metadata_iter->second.updateForInGroupKey(key); // TODO: update group-level metadata will affect other keys' popularities, while we use lazy update for those popularities (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead
        }
        else // Related with value
        {        
            pergroup_metadata_iter->second.updateForInGroupKeyValue(key, value, original_value); // TODO: update group-level metadata will affect other keys' popularities, while we use lazy update for those popularities (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead
        }

        return pergroup_metadata_iter->second;
    }

    void CacheMetadataBase::removePergroupMetadata_(const perkey_lookup_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const bool& is_local_cached_metadata)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_t::iterator perkey_metadata_iter = lookup_metadata.getPerkeyMetadataIter();
        assert(perkey_metadata_iter != perkey_metadata_list_.end()); // For existing key

        // NOTE: we should NOT decrease cur_group_keycnt_ here, which will make the number of GroupLevelMetadata in cur_group_id_ exceed COVERED_PERGROUP_MAXKEYCNT, if the degrouped key is tracked by previous groups (i.e., group ids < cur_group_id_)
        //cur_group_keycnt_--;

        GroupId tmp_group_id = perkey_metadata_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        // NOTE: for local cached metadata, as we always use accurate value sizes for group-level metadata of cached objects, we need to warn for valid avg_object_size_ and object_cnt_
        // NOTE: for local uncached metadata, although we have accurate value sizes when adding new objects into group-level metadata of uncached objects or detracking existing objects caused by cache admission, we still need to use approximate value sizes when replacing original value sizes with latest ones or detracking old objects caused by newly-tracked objects of local uncached metadata due to metadata capacity limitation -> disable warnings of invalid avg_object_size_ and object_cnt_ due to normal cases
        const bool need_warning = is_local_cached_metadata;
        bool is_group_empty = pergroup_metadata_iter->second.updateForDegrouped(key, value, need_warning); // TODO: update group-level metadata will affect other keys' popularities, while we use lazy update for those popularities (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead
        if (is_group_empty)
        {
            pergroup_metadata_map_.erase(pergroup_metadata_iter);
        }

        return;
    }

    // For popularity information

    Popularity CacheMetadataBase::getPopularity_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::const_iterator sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key

        Popularity popularity = sorted_popularity_iter->first;
        return popularity;
    }

    uint32_t CacheMetadataBase::getLeastPopularRank_(const perkey_lookup_const_iter_t& perkey_lookup_iter) const
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::const_iterator sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key

        uint32_t least_popular_rank = std::distance(sorted_popularity_multimap_.begin(), sorted_popularity_iter);
        assert(least_popular_rank < sorted_popularity_multimap_.size());
        
        return least_popular_rank;
    }

    Popularity CacheMetadataBase::calculatePopularity_(const KeyLevelMetadata& perkey_statistics, const GroupLevelMetadata& pergroup_statistics) const
    {
        // TODO: Use homogeneous popularity calculation now, but will replace with heterogeneous popularity calculation + learning later (for both local cached and uncached objects)
        // TODO: Use a heuristic or learning-based approach for parameter tuning to calculate local rewards for heterogeneous popularity calculation (refer to state-of-the-art studies such as LRB and GL-Cache)

        // NOTE: Here we use a simple approach to calculate popularity based on object-level and group-level metadata
        Popularity popularity = 0.0;
        #ifdef TRACK_PERKEY_OBJSIZE
        AvgObjectSize avg_objsize_bytes = static_cast<AvgObjectSize>(perkey_statistics.getObjectSize());
        #else
        AvgObjectSize avg_objsize_bytes = pergroup_statistics.getAvgObjectSize();
        #endif
        if (avg_objsize_bytes == 0.0) // Zero avg object size due to approximate value sizes in local uncached metadata
        {
            popularity = 0; // Set popularity as zero to avoid mis-admiting the uncached object with unknow object size
        }
        else
        {
            //Popularity avg_objsize_kb = Util::popularityDivide(static_cast<Popularity>(avg_objsize_bytes), 1024.0);
            popularity = Util::popularityDivide(static_cast<Popularity>(perkey_statistics.getFrequency()), avg_objsize_bytes); // # of cache accesses per space unit (similar as LHD)
        }

        return popularity;
    }

    sorted_popularity_multimap_t::iterator CacheMetadataBase::addPopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // NOTE: NO need to verify key existence due to duplicate popularity values
        // NOTE: perkey_metadata_iter_ and sorted_popularity_iter_ in perkey_lookup_iter here are invalid

        // Add new popularity information
        sorted_popularity_multimap_t::iterator new_popularity_iter = sorted_popularity_multimap_.insert(std::pair(new_popularity, perkey_lookup_iter->first));

        // Update size usage of popularity information
        sorted_popularity_multimap_key_size_ = Util::uint64Add(sorted_popularity_multimap_key_size_, perkey_lookup_iter->first.getKeyLength());

        return new_popularity_iter;
    }

    sorted_popularity_multimap_t::iterator CacheMetadataBase::updatePopularity_(const Popularity& new_popularity, const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::iterator old_sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(old_sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key
        assert(old_sorted_popularity_iter->second == perkey_lookup_iter->first);

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
        sorted_popularity_multimap_t::iterator new_popularity_iter = sorted_popularity_multimap_.insert(std::pair(new_popularity, perkey_lookup_iter->first));

        // NOTE: NO need to update size usage of popularity information

        return new_popularity_iter;
    }

    void CacheMetadataBase::removePopularity_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata& lookup_metadata = perkey_lookup_iter->second;
        sorted_popularity_multimap_t::iterator sorted_popularity_iter = lookup_metadata.getSortedPopularityIter();
        assert(sorted_popularity_iter != sorted_popularity_multimap_.end()); // For existing key
        assert(sorted_popularity_iter->second == perkey_lookup_iter->first);

        // Remove popularity information
        sorted_popularity_multimap_.erase(sorted_popularity_iter);

        // Update size usage of popularity information
        sorted_popularity_multimap_key_size_ = Util::uint64Minus(sorted_popularity_multimap_key_size_, perkey_lookup_iter->first.getKeyLength());

        return;
    }

    // For lookup table

    perkey_lookup_iter_t CacheMetadataBase::getLookup_(const Key& key)
    {
        perkey_lookup_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    perkey_lookup_const_iter_t CacheMetadataBase::getLookup_(const Key& key) const
    {
        perkey_lookup_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    perkey_lookup_iter_t CacheMetadataBase::tryToGetLookup_(const Key& key)
    {
        perkey_lookup_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);

        return perkey_lookup_iter;
    }

    perkey_lookup_const_iter_t CacheMetadataBase::tryToGetLookup_(const Key& key) const
    {
        perkey_lookup_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);

        return perkey_lookup_iter;
    }

    perkey_lookup_iter_t CacheMetadataBase::addLookup_(const Key& key)
    {
        // Verify that key must NOT exist (NOT admitted/tracked)
        perkey_lookup_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter == perkey_lookup_table_.end());

        // Add new lookup metadata
        perkey_lookup_iter = perkey_lookup_table_.insert(std::pair(key, LookupMetadata())).first;
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        // Update size usage of lookup table
        perkey_lookup_table_key_size_ = Util::uint64Add(perkey_lookup_table_key_size_, key.getKeyLength());

        return perkey_lookup_iter;
    }

    void CacheMetadataBase::updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const sorted_popularity_multimap_t::iterator& new_sorted_popularity_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setSortedPopularityIter(new_sorted_popularity_iter);

        // NOTE: NO need to update size usage of lookup table

        return;
    }

    void CacheMetadataBase::updateLookup_(const perkey_lookup_iter_t& perkey_lookup_iter, const perkey_metadata_list_t::iterator& perkey_metadata_iter, const sorted_popularity_multimap_t::iterator& sorted_popularity_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setPerkeyMetadataIter(perkey_metadata_iter);
        perkey_lookup_iter->second.setSortedPopularityIter(sorted_popularity_iter);

        // NOTE: NO need to update size usage of lookup table

        return;
    }

    void CacheMetadataBase::removeLookup_(const perkey_lookup_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // Update size usage of lookup table
        perkey_lookup_table_key_size_ = Util::uint64Minus(perkey_lookup_table_key_size_, perkey_lookup_iter->first.getKeyLength());

        perkey_lookup_table_.erase(perkey_lookup_iter);

        return;
    }
}