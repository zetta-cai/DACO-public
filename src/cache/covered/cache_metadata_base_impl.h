#ifndef CACHE_METADATA_BASE_IMPL_H
#define CACHE_METADATA_BASE_IMPL_H

#include "cache/covered/cache_metadata_base.h"

#include <assert.h>
#include <iterator> // std::distance

#include "common/util.h"

namespace covered
{
    // LookupMetadata

    template<class T>
    const std::string LookupMetadata<T>::kClassName("LookupMetadata");

    template<class T>
    LookupMetadata<T>::LookupMetadata() {}

    template<class T>
    LookupMetadata<T>::LookupMetadata(const LookupMetadata<T>& other)
    {
        perkey_metadata_list_iter_ = other.perkey_metadata_list_iter_;
        sorted_reward_iter_ = other.sorted_reward_iter_;
    }

    template<class T>
    LookupMetadata<T>::~LookupMetadata() {}

    template<class T>
    typename LookupMetadata<T>::perkey_metadata_list_iter_t LookupMetadata<T>::getPerkeyMetadataListIter() const
    {
        return perkey_metadata_list_iter_;
    }

    template<class T>
    sorted_reward_multimap_t::iterator LookupMetadata<T>::getSortedRewardIter() const
    {
        return sorted_reward_iter_;
    }

    template<class T>
    void LookupMetadata<T>::setPerkeyMetadataListIter(const typename LookupMetadata<T>::perkey_metadata_list_iter_t& perkey_metadata_list_iter)
    {
        perkey_metadata_list_iter_ = perkey_metadata_list_iter;
        return;
    }
    
    template<class T>
    void LookupMetadata<T>::setSortedRewardIter(const sorted_reward_multimap_t::iterator& sorted_reward_iter)
    {
        sorted_reward_iter_ = sorted_reward_iter;
        return;
    }

    template<class T>
    uint64_t LookupMetadata<T>::getPerkeyMetadataListIterSizeForCapacity()
    {
        return sizeof(perkey_metadata_list_iter_t);
    }

    template<class T>
    uint64_t LookupMetadata<T>::getSortedRewardIterSizeForCapacity()
    {
        return sizeof(sorted_reward_multimap_t::iterator);
    }

    template<class T>
    const LookupMetadata<T>& LookupMetadata<T>::operator=(const LookupMetadata<T>& other)
    {
        perkey_metadata_list_iter_ = other.perkey_metadata_list_iter_;
        sorted_reward_iter_ = other.sorted_reward_iter_;

        return *this;
    }

    // CacheMetadataBase

    template<class T>
    const std::string CacheMetadataBase<T>::kClassName("CacheMetadataBase");

    template<class T>
    CacheMetadataBase<T>::CacheMetadataBase()
    {
        perkey_metadata_list_key_size_ = 0;
        perkey_metadata_list_.clear();

        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_metadata_map_.clear();

        sorted_reward_multimap_key_size_ = 0;
        sorted_reward_multimap_.clear();

        perkey_lookup_table_key_size_ = 0;
        perkey_lookup_table_.clear();
    }

    template<class T>
    CacheMetadataBase<T>::~CacheMetadataBase() {}

    template<class T>
    bool CacheMetadataBase<T>::isKeyExist(const Key& key) const
    {
        perkey_lookup_table_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        return (perkey_lookup_iter != perkey_lookup_table_.end());
    }

    // For newly-admited/tracked keys

    template<class T>
    bool CacheMetadataBase<T>::addForNewKey(const Key& key, const Value& value, const uint32_t& peredge_synced_victimcnt, const bool& is_global_cached, const bool& is_neighbor_cached)
    {
        bool affect_victim_tracker = false;

        // Add lookup iterator for new key
        perkey_lookup_table_iter_t perkey_lookup_iter = addLookup_(key);

        // Add group-level metadata for all requests (both value-unrelated and value-related) for new key
        GroupId assigned_group_id = 0;
        const GroupLevelMetadata& group_level_metadata_ref = addPergroupMetadata_(key, value, assigned_group_id);

        // Add object-level metadata for local requests (both value-unrelated and value-related) for new key
        perkey_metadata_list_iter_t perkey_metadata_list_iter = addPerkeyMetadata_(key, value, assigned_group_id, is_global_cached, is_neighbor_cached);
        const T& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // Calculate and add reward for newly-admited key
        Reward new_reward = calculateReward_(perkey_metadata_list_iter);
        sorted_reward_multimap_t::iterator sorted_reward_iter = addReward_(new_reward, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, perkey_metadata_list_iter, sorted_reward_iter);

        // Post-processing after adding for new key: check if affect victim tracker for local cached metadata, or check space limitation for local uncached metadata
        affect_victim_tracker = afterAddForNewKey_(perkey_lookup_iter, peredge_synced_victimcnt);

        return affect_victim_tracker;
    }

    // For existing key

    template<class T>
    bool CacheMetadataBase<T>::updateNoValueStatsForExistingKey(const Key& key, const uint32_t& peredge_synced_victimcnt, const bool& is_redirected, const bool& is_global_cached)
    {
        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);

        bool affect_victim_tracker = beforeUpdateStatsForExistingKey_(perkey_lookup_iter, peredge_synced_victimcnt);

        // Update object-level value-unrelated metadata for local requests (local hits/misses)
        perkey_metadata_list_iter_t perkey_metadata_list_iter = updateNoValuePerkeyMetadata_(perkey_lookup_iter, is_redirected, is_global_cached);
        const T& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Update group-level value-unrelated metadata for all requests (local/redirected hits; local misses)
        const GroupLevelMetadata& group_level_metadata_ref = updateNoValuePergroupMetadata_(perkey_lookup_iter);

        // Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // Update reward
        Reward new_reward = calculateReward_(perkey_metadata_list_iter);
        sorted_reward_multimap_t::iterator new_sorted_reward_iter = updateReward_(new_reward, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, new_sorted_reward_iter);

        if (!affect_victim_tracker)
        {
            affect_victim_tracker = afterUpdateStatsForExistingKey_(perkey_lookup_iter, peredge_synced_victimcnt);
        }

        return affect_victim_tracker;
    }

    template<class T>
    bool CacheMetadataBase<T>::updateValueStatsForExistingKey(const Key& key, const Value& value, const Value& original_value, const uint32_t& peredge_synced_victimcnt)
    {
        // NOTE: NOT update object-/group-level value-unrelated metadata, which has been done in updateNoValueStatsForExistingKey()

        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);

        bool affect_victim_tracker = beforeUpdateStatsForExistingKey_(perkey_lookup_iter, peredge_synced_victimcnt);

        // Update object-level value-related metadata for local requests (local hits/misses)
        perkey_metadata_list_iter_t perkey_metadata_list_iter = updateValuePerkeyMetadata_(perkey_lookup_iter, value, original_value);
        const T& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Update group-level value-related metadata for all requests (local/redirected hits and local misses)
        const GroupLevelMetadata& group_level_metadata_ref = updateValuePergroupMetadata_(perkey_lookup_iter, key, value, original_value);

        // Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // Update reward
        Reward new_reward = calculateReward_(perkey_metadata_list_iter);
        sorted_reward_multimap_t::iterator new_sorted_reward_iter = updateReward_(new_reward, perkey_lookup_iter);

        // Update lookup table
        updateLookup_(perkey_lookup_iter, new_sorted_reward_iter);

        if (!affect_victim_tracker)
        {
            affect_victim_tracker = afterUpdateStatsForExistingKey_(perkey_lookup_iter, peredge_synced_victimcnt);
        }

        return affect_victim_tracker;
    }

    template<class T>
    void CacheMetadataBase<T>::removeForExistingKey(const Key& detracked_key, const Value& detracked_value)
    {
        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(detracked_key);

        // Remove group-level metadata
        removePergroupMetadata_(perkey_lookup_iter, detracked_key, detracked_value);

        // Remove object-level metadata
        removePerkeyMetadata_(perkey_lookup_iter);
        perkey_lookup_iter->second.setPerkeyMetadataListIter(perkey_metadata_list_.end());

        // Remove reward
        removeReward_(perkey_lookup_iter);
        perkey_lookup_iter->second.setSortedRewardIter(sorted_reward_multimap_.end());

        // Remove lookup table
        removeLookup_(perkey_lookup_iter);

        return;
    }

    // For object size

    template<class T>
    ObjectSize CacheMetadataBase<T>::getObjectSize(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_table_const_iter_t perkey_lookup_const_iter = getLookup_(key);

        // Get accurate/average object size
        ObjectSize object_size = getObjectSize_(perkey_lookup_const_iter);

        return object_size;
    }

    template<class T>
    ObjectSize CacheMetadataBase<T>::getObjectSize_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const
    {
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        const T& perkey_metadata_ref = getkeyLevelMetadata_(perkey_lookup_const_iter);
        ObjectSize object_size = perkey_metadata_ref.getObjectSize();
        #else
        const GroupLevelMetadata& pergroup_metadata_ref = getGroupLevelMetadata_(perkey_lookup_const_iter);
        ObjectSize object_size = static_cast<ObjectSize>(pergroup_metadata_ref.getAvgObjectSize());
        #endif

        return object_size;
    }

    template<class T>
    uint64_t CacheMetadataBase<T>::getSizeForCapacity_(const bool& is_local_cached_metadata) const
    {
        // Object-level metadata
        uint64_t perkey_metadata_list_metadata_size = perkey_metadata_list_.size() * T::getSizeForCapacity();

        // Group-level metadata
        uint64_t pergroup_metadata_map_groupid_size = pergroup_metadata_map_.size() * sizeof(GroupId);
        uint64_t pergroup_metadata_map_metadata_size = pergroup_metadata_map_.size() * GroupLevelMetadata::getSizeForCapacity();

        // Reward information
        uint64_t sorted_reward_multimap_reward_size = sorted_reward_multimap_.size() * sizeof(Reward);

        // Lookup table
        uint64_t perkey_lookup_table_perkey_metadata_list_iter_size = perkey_lookup_table_.size() * LookupMetadata<perkey_metadata_list_t>::getPerkeyMetadataListIterSizeForCapacity();
        uint64_t perkey_lookup_table_sorted_reward_iter_size = perkey_lookup_table_.size() * LookupMetadata<perkey_metadata_list_t>::getSortedRewardIterSizeForCapacity();

        uint64_t total_size = 0;

        if (is_local_cached_metadata) // For local cached metadata
        {
            // Object-level metadata
            // NOTE: (i) LRU list does NOT need to track keys; (ii) although we track local cached metadata outside CacheLib to avoid extensive hacking, per-key object-level metadata actually can be stored into cachelib::CacheItem -> NO need to count the size of keys of object-level metadata for local cached objects (similar as size measurement in ValidityMap and BlockTracker)
            //total_size = Util::uint64Add(total_size, perkey_metadata_list_key_size_);
            total_size = Util::uint64Add(total_size, perkey_metadata_list_metadata_size);

            // Group-level metadata
            total_size = Util::uint64Add(total_size, pergroup_metadata_map_groupid_size);
            total_size = Util::uint64Add(total_size, pergroup_metadata_map_metadata_size);

            // Reward information (locate keys in lookup table for reward-based eviction)
            // NOTE: we only need to maintain keys (or CacheItem pointers) in reward list instead of LRU list (we NEVER use perkey_metadata_list_->first to locate keys) for reward-based eviction, while cachelib has counted the size of keys (i.e., CacheItem pointers) in MMContainer's LRU list (we do NOT remove keys from LRU list in cachelib to avoid hacking too much code) -> NO need to count the size of keys of reward list for local cached objects here
            total_size = Util::uint64Add(total_size, sorted_reward_multimap_reward_size);
            //total_size = Util::uint64Add(total_size, sorted_reward_multimap_key_size_);

            // Lookup table (locate keys in object-level LRU list and reward list)
            // NOTE: although we track local cached metadata outside CacheLib to avoid extensive hacking, per-key lookup metadata actually can be stored into cachelib::ChainedHashTable -> NO need to count the size of keys and object-level metadata iterators of lookup metadata for local cached objects
            //total_size = Util::uint64Add(total_size, perkey_lookup_table_key_size_);
            //total_size = Util::uint64Add(total_size, perkey_lookup_table_perkey_metadata_list_iter_size); // LRU list iterator
            UNUSED(perkey_lookup_table_perkey_metadata_list_iter_size);
            total_size = Util::uint64Add(total_size, perkey_lookup_table_sorted_reward_iter_size); // Reward list iterator
        }
        else // For local uncached metadata
        {
            // Object-level metadata
            // NOTE: although we maintain keys in perkey_metadata_list_, it is just for implementation simplicity (e.g., key can be replaced by a pointer referring to the corresponding lookup table entry) but we NEVER use perkey_metadata_list_->first to locate keys (due to reward-based eviciton instead of LRU-based eviction) -> NO need to count the size of keys of object-level metadata for local uncached objects (cache size usage of keys will be counted in lookup table below)
            //total_size = Util::uint64Add(total_size, perkey_metadata_list_key_size_);
            total_size = Util::uint64Add(total_size, perkey_metadata_list_metadata_size);

            // Group-level metadata
            total_size = Util::uint64Add(total_size, pergroup_metadata_map_groupid_size);
            total_size = Util::uint64Add(total_size, pergroup_metadata_map_metadata_size);

            // Reward information (locate keys in lookup table for reward-based eviction)
            total_size = Util::uint64Add(total_size, sorted_reward_multimap_reward_size);
            total_size = Util::uint64Add(total_size, sorted_reward_multimap_key_size_);

            // Lookup table (locate keys in object-level LRU list and reward list)
            total_size = Util::uint64Add(total_size, perkey_lookup_table_key_size_);
            total_size = Util::uint64Add(total_size, perkey_lookup_table_perkey_metadata_list_iter_size); // LRU list iterator
            total_size = Util::uint64Add(total_size, perkey_lookup_table_sorted_reward_iter_size); // Reward list iterator
        }

        return total_size;
    }

    // For object-level metadata

    template<class T>
    const T& CacheMetadataBase<T>::getkeyLevelMetadata(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_table_const_iter_t perkey_lookup_const_iter = getLookup_(key);
        
        return getkeyLevelMetadata_(perkey_lookup_const_iter);
    }

    template<class T>
    const T& CacheMetadataBase<T>::getkeyLevelMetadata_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const
    {
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_const_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // NOTE: object-level metadata MUST exist for the key

        // Get object-level metadata for local requests (local hits/misses)
        return perkey_metadata_list_iter->second;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_metadata_list_iter_t CacheMetadataBase<T>::addPerkeyMetadata_(const Key& key, const Value& value, const GroupId& assigned_group_id, const bool& is_global_cached, const bool& is_neighbor_cached)
    {
        // NOTE: NO need to verify key existence due to LRU-based list

        // Add object-level metadata for new key
        perkey_metadata_list_.push_front(std::pair<Key, T>(key, T(assigned_group_id, is_neighbor_cached)));
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_metadata_list_.begin();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end());

        // Update both value-unrelated and value-related metadata for new key
        const bool is_redirected = false; // ONLY local-request-related messages (e.g., directory lookup, foreground directory eviction, acquire/release writelock; getrsp/put/delreq w/ local misses) can admit/track new keys in local cached/uncached metadata
        const ObjectSize object_size = key.getKeyLength() + value.getValuesize();
        perkey_metadata_list_iter->second.updateNoValueDynamicMetadata(is_redirected, is_global_cached);
        perkey_metadata_list_iter->second.updateValueDynamicMetadata(object_size, 0);
        // NOTE: local popularity of the key-level metadata will be updated by addForNewKey()

        // push_front already places the new object-level metadata to the head of LRU list -> NO need to update LRU list order

        // Update size usage of key-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Add(perkey_metadata_list_key_size_, key.getKeyLength());

        return perkey_metadata_list_iter;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_metadata_list_iter_t CacheMetadataBase<T>::updateNoValuePerkeyMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const bool& is_redirected, const bool& is_global_cached)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // Update object-level value-unrelated metadata
        perkey_metadata_list_iter->second.updateNoValueDynamicMetadata(is_redirected, is_global_cached);

        // Update LRU list order
        if (perkey_metadata_list_iter != perkey_metadata_list_.begin())
        {
            // Move the list entry pointed by perkey_metadata_list_iter to the head of the list (NOT change the memory address of the list entry)
            perkey_metadata_list_.splice(perkey_metadata_list_.begin(), perkey_metadata_list_, perkey_metadata_list_iter);
        }

        // NOTE: local popularity of the key-level metadata will be updated by updateNoValueStatsForExistingKey()

        // NOTE: NO need to update size usage of key-level metadata

        return perkey_metadata_list_iter;
    }
    
    template<class T>
    typename CacheMetadataBase<T>::perkey_metadata_list_iter_t CacheMetadataBase<T>::updateValuePerkeyMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const Value& value, const Value& original_value)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // Update object-level value-related metadata
        const ObjectSize tmp_object_size = perkey_lookup_iter->first.getKeyLength() + value.getValuesize();
        const ObjectSize tmp_original_object_size = perkey_lookup_iter->first.getKeyLength() + original_value.getValuesize();
        perkey_metadata_list_iter->second.updateValueDynamicMetadata(tmp_object_size, tmp_original_object_size);

        // NOTE: NOT update LRU list order which has been done in updateNoValuePerkeyMetadata_()

        // NOTE: local popularity of the key-level metadata will be updated by updateValueStatsForExistingKey_()

        // NOTE: NO need to update size usage of key-level metadata

        return perkey_metadata_list_iter;
    }

    template<class T>
    void CacheMetadataBase<T>::removePerkeyMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // Update size usage of key-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Minus(perkey_metadata_list_key_size_, perkey_metadata_list_iter->first.getKeyLength());

        perkey_metadata_list_.erase(perkey_metadata_list_iter);

        // NOTE: erase removes the object-level metadata yet NOT affect other iterators -> NO need to update LRU list order

        return;
    }

    // For group-level metadata

    template<class T>
    const GroupLevelMetadata& CacheMetadataBase<T>::getGroupLevelMetadata_(const perkey_lookup_table_const_iter_t& perkey_lookup_const_iter) const
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_const_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        GroupId tmp_group_id = perkey_metadata_list_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::const_iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        return pergroup_metadata_iter->second;
    }

    template<class T>
    const GroupLevelMetadata& CacheMetadataBase<T>::addPergroupMetadata_(const Key& key, const Value& value, GroupId& assigned_group_id)
    {
        // NOTE: NO need to verify key existence due to duplicate group IDs

        // Assign group ID for new key
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

        // Update both value-unrelated and value-related metadata for newly-grouped key
        pergroup_metadata_iter->second.updateForNewlyGrouped(key, value); // TODO: update group-level metadata will affect other keys' popularities and rewards, while we use lazy update (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead

        return pergroup_metadata_iter->second;
    }

    template<class T>
    const GroupLevelMetadata& CacheMetadataBase<T>::updateNoValuePergroupMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // Update group-level value-unrelated metadata
        GroupId tmp_group_id = perkey_metadata_list_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        pergroup_metadata_iter->second.updateNoValueStatsForInGroupKey(); // TODO: update group-level metadata will affect other keys' popularities and rewards, while we use lazy update (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead

        return pergroup_metadata_iter->second;
    }

    template<class T>
    const GroupLevelMetadata& CacheMetadataBase<T>::updateValuePergroupMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const Key& key, const Value& value, const Value& original_value)
    {        
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // Update group-level value-related metadata
        GroupId tmp_group_id = perkey_metadata_list_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        pergroup_metadata_iter->second.updateValueStatsForInGroupKey(key, value, original_value); // TODO: update group-level metadata will affect other keys' popularities and rewards, while we use lazy update (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead

        return pergroup_metadata_iter->second;
    }

    template<class T>
    void CacheMetadataBase<T>::removePergroupMetadata_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const Key& key, const Value& value)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        perkey_metadata_list_iter_t perkey_metadata_list_iter = lookup_metadata.getPerkeyMetadataListIter();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key

        // NOTE: we should NOT decrease cur_group_keycnt_ here, which will make the number of GroupLevelMetadata in cur_group_id_ exceed COVERED_PERGROUP_MAXKEYCNT, if the degrouped key is tracked by previous groups (i.e., group ids < cur_group_id_)
        //cur_group_keycnt_--;

        GroupId tmp_group_id = perkey_metadata_list_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        // NOTE: for local cached metadata, as we always use accurate value sizes for group-level metadata of cached objects, we need to warn for valid avg_object_size_ and object_cnt_
        // NOTE: for local uncached metadata, although we have accurate value sizes when adding new objects into group-level metadata of uncached objects or detracking existing objects caused by cache admission, we still need to use approximate value sizes when replacing original value sizes with latest ones or detracking old objects caused by newly-tracked objects of local uncached metadata due to metadata capacity limitation -> disable warnings of invalid avg_object_size_ and object_cnt_ due to normal cases
        bool is_group_empty = pergroup_metadata_iter->second.updateForDegrouped(key, value); // TODO: update group-level metadata will affect other keys' popularities and rewards, while we use lazy update (i.e., update them when they are accessed) to avoid maintaining groupid-keys mappings for limited metadata overhead
        if (is_group_empty)
        {
            pergroup_metadata_map_.erase(pergroup_metadata_iter);
        }

        return;
    }

    // For local (cached/uncached) popularity (local hits for local cached metadata; local misses for local uncached metadata)

    template<class T>
    Popularity CacheMetadataBase<T>::calculatePopularity_(const Frequency& frequency, const ObjectSize& object_size) const
    {
        // (OBSOLETE: zero-reward for one-hit-wonders will mis-evict hot keys) Set popularity as zero for zero-reward of one-hit-wonders to quickly evict them
        // if (frequency <= 1)
        // {
        //     return 0;
        // }

        ObjectSize tmp_object_size = object_size;

        // (OBSOLETE: we CANNOT directly use recency_index, as each object has an recency_index of 1 when updating popularity and we will NOT update recency info of all objects for each cache hit/miss)
        //uint32_t recency_index = std::distance(perkey_metadata_list_.begin(), perkey_metadata_const_iter) + 1;
        //tmp_object_size *= recency_index;

        // NOTE: Here we use a simple approach to calculate popularity
        Popularity popularity = 0.0;

        if (tmp_object_size == 0) // Zero object size due to delreqs or approximate value sizes in local uncached metadata
        {
            #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
            assert(false); // TMPDEBUG23
            tmp_object_size = 1; // Give the largest possible popularity for delreqs due to zero space usage for deleted value
            #else
            popularity = 0; // Set popularity as zero to avoid mis-admiting the uncached object with unknow object size if w/ approximate value sizes
            return popularity;
            #endif
        }
        
        //ObjectSize tmp_objsize_kb = B2KB(tmp_object_size);
        popularity = Util::popularityDivide(static_cast<Popularity>(frequency), static_cast<Popularity>(tmp_object_size)); // # of cache accesses per space unit (similar as LHD)

        return popularity;
    }

    // For reward information

    template<class T>
    Reward CacheMetadataBase<T>::getLocalRewardForExistingKey(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = getLookup_(key);

        // For existing key
        sorted_reward_multimap_t::const_iterator sorted_reward_iter = lookup_metadata.getSortedRewardIter();
        assert(sorted_reward_iter != sorted_reward_multimap_.end());

        return sorted_reward_multimap_->first;
    }

    template<class T>
    bool CacheMetadataBase<T>::getLeastRewardKeyAndReward(const uint32_t& least_reward_rank, Key& key, Reward& reward) const
    {
        bool is_least_reward_key_exist = false;

        if (least_reward_rank < sorted_reward_multimap_.size())
        {
            // // MRU for equal reward values (especially for zero-reward one-hit-wonders)
            // sorted_reward_multimap_t::const_iterator sorted_reward_iter = sorted_reward_multimap_.end();
            // std::advance(sorted_reward_iter, -1 * static_cast<int>(least_reward_rank + 1));

            // LRU for equal reward values
            sorted_reward_multimap_t::const_iterator sorted_reward_iter = sorted_reward_multimap_.begin();
            std::advance(sorted_reward_iter, least_reward_rank);

            key = sorted_reward_iter->second;
            reward = sorted_reward_iter->first;

            is_least_reward_key_exist = true;
        }

        return is_least_reward_key_exist;
    }

    template<class T>
    uint32_t CacheMetadataBase<T>::getLeastRewardRank_(const typename CacheMetadataBase<T>::perkey_lookup_table_const_iter_t& perkey_lookup_iter) const
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        sorted_reward_multimap_t::const_iterator sorted_reward_iter = lookup_metadata.getSortedRewardIter();
        assert(sorted_reward_iter != sorted_reward_multimap_.end()); // For existing key

        // // MRU for equal reward values (especially for zero-reward one-hit-wonders)
        // uint32_t least_reward_rank = std::distance(sorted_reward_iter, sorted_reward_multimap_.end()); // NOTE: std::distance MUST from previous iter to subsequent iter
        // assert(least_reward_rank >= 1);
        // least_reward_rank -= 1;

        // LRU for equal reward values
        uint32_t least_reward_rank = std::distance(sorted_reward_multimap_.begin(), sorted_reward_iter);

        assert(least_reward_rank < sorted_reward_multimap_.size());
        
        return least_reward_rank;
    }

    template<class T>
    sorted_reward_multimap_t::iterator CacheMetadataBase<T>::addReward_(const Reward& new_reward, const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // NOTE: NO need to verify key existence due to duplicate reward values
        // NOTE: perkey_metadata_list_iter_ and sorted_reward_iter_ in perkey_lookup_iter here are invalid

        // Add new reward information
        sorted_reward_multimap_t::iterator new_reward_iter = sorted_reward_multimap_.insert(std::pair(new_reward, perkey_lookup_iter->first));

        // Update size usage of reward information
        sorted_reward_multimap_key_size_ = Util::uint64Add(sorted_reward_multimap_key_size_, perkey_lookup_iter->first.getKeyLength());

        return new_reward_iter;
    }

    template<class T>
    sorted_reward_multimap_t::iterator CacheMetadataBase<T>::updateReward_(const Reward& new_reward, const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        sorted_reward_multimap_t::iterator old_sorted_reward_iter = lookup_metadata.getSortedRewardIter();
        assert(old_sorted_reward_iter != sorted_reward_multimap_.end()); // For existing key
        assert(old_sorted_reward_iter->second == perkey_lookup_iter->first);

        /*// Get handle referring to item by move assignment operator
        // NOTE: now handle points to item, yet old_sorted_reward_iter->second points to NULL)
        LruCacheReadHandle handle = std::move(old_sorted_reward_iter->second);*/

        // Remove old reward information
        sorted_reward_multimap_.erase(old_sorted_reward_iter);
        
        /*// Create pair for new reward by std::pair<T1, T2>(T1&&, T2&&) -> move constructor of T1 and T2
        // NOTE: now tmp_pair->second points to item, yet handle points to NULL
        std::pair<Reward, LruCacheReadHandle> tmp_pair(std::move(new_reward), std::move(handle));

        // Add new reward information by std::multimap::insert(value_type&&) -> move constructor of std::pair<T1, T2>(std::pair&&) -> move constructor of T1 and T2 (note that members of rvalue is still rvalue)
        // NOTE: now new_reward_iter points to item, yet tmp_pair->second points to NULL
        multimap_iterator_t new_reward_iter = sorted_reward_multimap_.insert(std::move(tmp_pair));*/

        // Add new reward information
        sorted_reward_multimap_t::iterator new_reward_iter = sorted_reward_multimap_.insert(std::pair(new_reward, perkey_lookup_iter->first));

        // NOTE: NO need to update size usage of reward information

        return new_reward_iter;
    }

    template<class T>
    void CacheMetadataBase<T>::removeReward_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        const LookupMetadata<perkey_metadata_list_t>& lookup_metadata = perkey_lookup_iter->second;
        sorted_reward_multimap_t::iterator sorted_reward_iter = lookup_metadata.getSortedRewardIter();
        assert(sorted_reward_iter != sorted_reward_multimap_.end()); // For existing key
        assert(sorted_reward_iter->second == perkey_lookup_iter->first);

        // Remove rewad information
        sorted_reward_multimap_.erase(sorted_reward_iter);

        // Update size usage of reward information
        sorted_reward_multimap_key_size_ = Util::uint64Minus(sorted_reward_multimap_key_size_, perkey_lookup_iter->first.getKeyLength());

        return;
    }

    // For lookup table

    template<class T>
    typename CacheMetadataBase<T>::perkey_lookup_table_iter_t CacheMetadataBase<T>::getLookup_(const Key& key)
    {
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        return perkey_lookup_iter;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_lookup_table_const_iter_t CacheMetadataBase<T>::getLookup_(const Key& key) const
    {
        perkey_lookup_table_const_iter_t perkey_lookup_const_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_const_iter != perkey_lookup_table_.end());

        return perkey_lookup_const_iter;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_lookup_table_iter_t CacheMetadataBase<T>::tryToGetLookup_(const Key& key)
    {
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);

        return perkey_lookup_iter;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_lookup_table_const_iter_t CacheMetadataBase<T>::tryToGetLookup_(const Key& key) const
    {
        perkey_lookup_table_const_iter_t perkey_lookup_const_iter = perkey_lookup_table_.find(key);

        return perkey_lookup_const_iter;
    }

    template<class T>
    typename CacheMetadataBase<T>::perkey_lookup_table_iter_t CacheMetadataBase<T>::addLookup_(const Key& key)
    {
        // Verify that key must NOT exist (NOT admitted/tracked)
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter == perkey_lookup_table_.end());

        // Add new lookup metadata
        perkey_lookup_iter = perkey_lookup_table_.insert(std::pair(key, LookupMetadata<perkey_metadata_list_t>())).first;
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        // Update size usage of lookup table
        perkey_lookup_table_key_size_ = Util::uint64Add(perkey_lookup_table_key_size_, key.getKeyLength());

        return perkey_lookup_iter;
    }

    template<class T>
    void CacheMetadataBase<T>::updateLookup_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const sorted_reward_multimap_t::iterator& new_sorted_reward_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setSortedRewardIter(new_sorted_reward_iter);

        // NOTE: NO need to update size usage of lookup table

        return;
    }

    template<class T>
    void CacheMetadataBase<T>::updateLookup_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter, const perkey_metadata_list_iter_t& perkey_metadata_list_iter, const sorted_reward_multimap_t::iterator& sorted_reward_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        perkey_lookup_iter->second.setPerkeyMetadataListIter(perkey_metadata_list_iter);
        perkey_lookup_iter->second.setSortedRewardIter(sorted_reward_iter);

        // NOTE: NO need to update size usage of lookup table

        return;
    }

    template<class T>
    void CacheMetadataBase<T>::removeLookup_(const typename CacheMetadataBase<T>::perkey_lookup_table_iter_t& perkey_lookup_iter)
    {
        // Verify that key must exist
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // Update size usage of lookup table
        perkey_lookup_table_key_size_ = Util::uint64Minus(perkey_lookup_table_key_size_, perkey_lookup_iter->first.getKeyLength());

        perkey_lookup_table_.erase(perkey_lookup_iter);

        return;
    }
}

#endif