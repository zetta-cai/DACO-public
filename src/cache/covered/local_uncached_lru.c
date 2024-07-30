#include "cache/covered/local_uncached_lru.h"

#include <assert.h>

#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"

namespace covered
{
    const std::string LocalUncachedLru::kClassName("LocalUncachedLru");

    LocalUncachedLru::LocalUncachedLru(const uint64_t& max_bytes_for_local_uncached_lru) : max_bytes_for_local_uncached_lru_(max_bytes_for_local_uncached_lru)
    {
        perkey_metadata_list_key_size_ = 0;
        perkey_metadata_list_.clear();

        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_metadata_map_.clear();

        perkey_lookup_table_key_size_ = 0;
        perkey_lookup_table_.clear();
    }

    LocalUncachedLru::~LocalUncachedLru() {}

    ObjectSize LocalUncachedLru::getObjectSize(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_table_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); 
        perkey_metadata_list_t::const_iterator perkey_metadata_const_iter = perkey_lookup_iter->second;

        // Get accurate/average object size
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize object_size = perkey_metadata_const_iter->second.getObjectSize();
        #else
        GroupId tmp_group_id = perkey_metadata_const_iter->second.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        ObjectSize object_size = static_cast<ObjectSize>(pergroup_metadata_iter->getAvgObjectSize());
        #endif

        return object_size;
    }

    bool LocalUncachedLru::isKeyExist(const Key& key) const
    {
        perkey_lookup_table_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        return (perkey_lookup_iter != perkey_lookup_table_.end());
    }

    bool LocalUncachedLru::isGlobalCachedForExistingKey(const Key& key) const
    {
        perkey_lookup_table_const_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); 
        perkey_metadata_list_t::const_iterator perkey_metadata_const_iter = perkey_lookup_iter->second;
        return perkey_metadata_const_iter->second.isGlobalCached();
    }

    Reward LocalUncachedLru::addForNewKey(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const Value& value, const bool& is_global_cached)
    {
        bool affect_victim_tracker = false;

        // (1) Add lookup table

        // Verify that key must NOT exist in the small LRU cache
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter == perkey_lookup_table_.end());

        // Add new lookup metadata
        perkey_lookup_iter = perkey_lookup_table_.insert(std::pair(key, perkey_metadata_list_iter_t())).first;
        assert(perkey_lookup_iter != perkey_lookup_table_.end());

        // Update size usage of lookup table
        perkey_lookup_table_key_size_ = Util::uint64Add(perkey_lookup_table_key_size_, key.getKeyLength());

        // (2) Add group-level metadata for local misses (both value-unrelated and value-related) for new key

        // Assign group ID
        cur_group_keycnt_++;
        if (cur_group_keycnt_ > COVERED_PERGROUP_MAXKEYCNT)
        {
            cur_group_id_++;
            cur_group_keycnt_ = 1;
        }
        GroupId assigned_group_id = cur_group_id_;

        // Add group-level metadata for new key
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(assigned_group_id);
        if (pergroup_metadata_iter == pergroup_metadata_map_.end())
        {
            pergroup_metadata_iter = pergroup_metadata_map_.insert(std::pair(assigned_group_id, GroupLevelMetadata())).first;
        }
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());

        // Update both value-unrelated and value-related metadata for newly-grouped key
        GroupLevelMetadata& group_level_metadata_ref = pergroup_metadata_iter->second;
        group_level_metadata_ref.updateForNewlyGrouped(key, value);

        // (3) Add object-level metadata for local requests (both value-unrelated and value-related) for new key

        // Add object-level metadata for new key
        const bool is_neighbor_cached = false; // NEVER used by local uncached objects
        perkey_metadata_list_.push_front(std::pair<Key, HomoKeyLevelMetadata>(key, HomoKeyLevelMetadata(assigned_group_id, is_neighbor_cached))); // Add to the head of the LRU list
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_metadata_list_.begin();
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end());
        HomoKeyLevelMetadata& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Update both value-unrelated and value-related metadata for new key
        const bool is_redirected = false; // ONLY local misses for local uncached objects
        const ObjectSize object_size = key.getKeyLength() + value.getValuesize();
        perkey_metadata_list_iter->second.updateNoValueDynamicMetadata(is_redirected, is_global_cached); // Add local freq by 1
        perkey_metadata_list_iter->second.updateValueDynamicMetadata(object_size, 0);

        // Update size usage of key-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Add(perkey_metadata_list_key_size_, key.getKeyLength());

        // (4) Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // (5) Calculate reward
        Reward new_reward = calculateReward_(edge_wrapper_ptr, perkey_metadata_list_iter);

        // (6) Update lookup table
        perkey_lookup_iter->second = perkey_metadata_list_iter;

        // (7) Evict key for local uncached LRU space limitation if necessary
        Key evicted_key;
        while (true)
        {
            bool need_evict = needEraseForUncachedObjects_(evicted_key);
            if (need_evict) // Cache size usage for local uncached objects in the small LRU cache exceeds the max bytes limitation
            {
                const uint32_t evicted_keylen = evicted_key.getKeyLength();
                const uint32_t evicted_object_size = getObjectSize(evicted_key);
                uint32_t evicted_value_size = evicted_object_size >= evicted_keylen ? evicted_object_size - evicted_keylen: 0;
                HomoKeyLevelMetadata unused_keylevel_metadata;
                removeForExistingKey(evicted_key, Value(evicted_value_size), unused_keylevel_metadata);
            }
            else // Local uncached objects in the small LRU cache is limited
            {
                break;
            }
        }

        return new_reward;
    }

    Reward LocalUncachedLru::updateNoValueStatsForExistingKey(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const bool& is_global_cached)
    {
        // (1) Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // (2) Update object-level value-unrelated metadata for local misses
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_lookup_iter->second;
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key
        HomoKeyLevelMetadata& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Update object-level value-unrelated metadata
        const bool is_redirected = false; // Must be false for local misses of local uncached objects
        key_level_metadata_ref.updateNoValueDynamicMetadata(is_redirected, is_global_cached);

        // Update LRU list order
        if (perkey_metadata_list_iter != perkey_metadata_list_.begin())
        {
            // Move the list entry pointed by perkey_metadata_list_iter to the head of the list (NOT change the memory address of the list entry)
            perkey_metadata_list_.splice(perkey_metadata_list_.begin(), perkey_metadata_list_, perkey_metadata_list_iter);
        }

        // (3) Update group-level value-unrelated metadata for local misses
        GroupId tmp_group_id = key_level_metadata_ref.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        GroupLevelMetadata& group_level_metadata_ref = pergroup_metadata_iter->second;
        group_level_metadata_ref.updateNoValueStatsForInGroupKey();

        // (4) Calculate and update popularity for newly-admited key
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // (5) Update reward
        Reward new_reward = calculateReward_(edge_wrapper_ptr, perkey_metadata_list_iter);

        return new_reward;
    }

    Reward LocalUncachedLru::updateValueStatsForExistingKey(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const Value& value, const Value& original_value)
    {
        // NOTE: NOT update object-/group-level value-unrelated metadata, which has been done in updateNoValueStatsForExistingKey()

        // (1) Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // (2) Update object-level value-related metadata for local misses
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_lookup_iter->second;
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key
        HomoKeyLevelMetadata& key_level_metadata_ref = perkey_metadata_list_iter->second;

        // Update object-level value-related metadata
        const ObjectSize tmp_object_size = perkey_lookup_iter->first.getKeyLength() + value.getValuesize();
        const ObjectSize tmp_original_object_size = perkey_lookup_iter->first.getKeyLength() + original_value.getValuesize();
        key_level_metadata_ref.updateValueDynamicMetadata(tmp_object_size, tmp_original_object_size);

        // NOTE: NOT update LRU list order which has been done in updateNoValueStatsForExistingKey()

        // (3) Update group-level value-related metadata for all requests (local/redirected hits and local misses)
        GroupId tmp_group_id = key_level_metadata_ref.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        GroupLevelMetadata& group_level_metadata_ref = pergroup_metadata_iter->second;
        group_level_metadata_ref.updateValueStatsForInGroupKey(key, value, original_value);

        // (4) Calculate and update popularity
        calculateAndUpdatePopularity_(perkey_metadata_list_iter, key_level_metadata_ref, group_level_metadata_ref);

        // (5) Update reward
        Reward new_reward = calculateReward_(edge_wrapper_ptr, perkey_metadata_list_iter);

        return new_reward;
    }

    void LocalUncachedLru::removeForExistingKey(const Key& key, const Value& value, HomoKeyLevelMetadata& homo_keylevel_metadata)
    {
        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // Get object-level metadata
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_lookup_iter->second;
        assert(perkey_metadata_list_iter != perkey_metadata_list_.end()); // For existing key
        homo_keylevel_metadata = perkey_metadata_list_iter->second;

        // Get group-level metadata
        GroupId tmp_group_id = homo_keylevel_metadata.getGroupId(); // Get group ID
        pergroup_metadata_map_t::iterator pergroup_metadata_iter = pergroup_metadata_map_.find(tmp_group_id);
        assert(pergroup_metadata_iter != pergroup_metadata_map_.end());
        GroupLevelMetadata& group_level_metadata_ref = pergroup_metadata_iter->second;

        // Remove group-level metadata
        bool is_group_empty = group_level_metadata_ref.updateForDegrouped(key, value);
        if (is_group_empty)
        {
            pergroup_metadata_map_.erase(pergroup_metadata_iter);
        }

        // Update size usage of key-level metadata and remove object-level metadata
        perkey_metadata_list_key_size_ = Util::uint64Minus(perkey_metadata_list_key_size_, perkey_metadata_list_iter->first.getKeyLength());
        perkey_metadata_list_.erase(perkey_metadata_list_iter);

        // Update size usage of lookup table and remove lookup table
        perkey_lookup_table_key_size_ = Util::uint64Minus(perkey_lookup_table_key_size_, perkey_lookup_iter->first.getKeyLength());
        perkey_lookup_table_.erase(perkey_lookup_iter);

        return;
    }

    Reward LocalUncachedLru::updateIsGlobalCachedForExistingKey(const EdgeWrapperBase* edge_wrapper_ptr, const Key& key, const bool& is_getrsp, const bool& is_global_cached)
    {
        assert(is_getrsp == true);

        // Get lookup iterator
        perkey_lookup_table_iter_t perkey_lookup_iter = perkey_lookup_table_.find(key);
        assert(perkey_lookup_iter != perkey_lookup_table_.end()); // For existing key

        // Update object-level is_global_cached flag of value-unrelated metadata for local getrsp with cache miss
        perkey_metadata_list_iter_t perkey_metadata_list_iter = perkey_lookup_iter->second;
        assert(is_global_cached != perkey_metadata_list_iter->second.isGlobalCached()); // is_global_cached flag MUST be changed
        perkey_metadata_list_iter->second.updateIsGlobalCached(is_global_cached);

        // NOTE: is_global_cached does NOT affect local popularity of HomoKeyLevelMetadata

        // Update reward
        Reward new_reward = calculateReward_(edge_wrapper_ptr, perkey_metadata_list_iter);

        return new_reward;
    }

    uint64_t LocalUncachedLru::getSizeForCapacity() const
    {
        // Object-level metadata
        uint64_t perkey_metadata_list_metadata_size = perkey_metadata_list_.size() * HomoKeyLevelMetadata::getSizeForCapacity();

        // Group-level metadata
        uint64_t pergroup_metadata_map_groupid_size = pergroup_metadata_map_.size() * sizeof(GroupId);
        uint64_t pergroup_metadata_map_metadata_size = pergroup_metadata_map_.size() * GroupLevelMetadata::getSizeForCapacity();

        // Lookup table
        uint64_t perkey_lookup_table_perkey_metadata_list_iter_size = perkey_lookup_table_.size() * sizeof(perkey_metadata_list_iter_t);

        uint64_t total_size = 0; // Size cost for local uncached metadata in the small LRU cache

        // Object-level metadata (locate keys in lookup table for LRU-based eviction)
        total_size = Util::uint64Add(total_size, perkey_metadata_list_key_size_);
        total_size = Util::uint64Add(total_size, perkey_metadata_list_metadata_size);

        // Group-level metadata
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_groupid_size);
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_metadata_size);

        // Lookup table (locate keys in object-level LRU list)
        total_size = Util::uint64Add(total_size, perkey_lookup_table_key_size_);
        total_size = Util::uint64Add(total_size, perkey_lookup_table_perkey_metadata_list_iter_size); // LRU list iterator

        return total_size;
    }

    void LocalUncachedLru::dumpLocalUncachedLru(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump object-level metadata
        // (1) perkey_metadata_list_key_size_
        fs_ptr->write((const char*)&perkey_metadata_list_key_size_, sizeof(uint64_t));
        // (2) key-KeyLevelMetadata pairs
        const uint32_t perkey_metadata_list_size = perkey_metadata_list_.size();
        fs_ptr->write((const char*)&perkey_metadata_list_size, sizeof(uint32_t));
        for (typename perkey_metadata_list_t::const_iterator perkey_metadata_list_const_iter = perkey_metadata_list_.begin(); perkey_metadata_list_const_iter != perkey_metadata_list_.end(); perkey_metadata_list_const_iter++)
        {
            // Dump the key
            const Key& tmp_key = perkey_metadata_list_const_iter->first;
            const uint32_t key_serialize_size = tmp_key.serialize(fs_ptr);

            // Dump KeyLevelMetadata
            const HomoKeyLevelMetadata& key_level_metadata = perkey_metadata_list_const_iter->second;
            key_level_metadata.dumpKeyLevelMetadata(fs_ptr);
        }

        // Dump group-level metadata
        // (1) cur_group_id_
        fs_ptr->write((const char*)&cur_group_id_, sizeof(GroupId));
        // (2) cur_group_keycnt_
        fs_ptr->write((const char*)&cur_group_keycnt_, sizeof(uint32_t));
        // (3) groupid-GroupLevelMetadata pairs
        const uint32_t pergroup_metadata_map_size = pergroup_metadata_map_.size();
        fs_ptr->write((const char*)&pergroup_metadata_map_size, sizeof(uint32_t));
        for (pergroup_metadata_map_t::const_iterator pergroup_metadata_map_const_iter = pergroup_metadata_map_.begin(); pergroup_metadata_map_const_iter != pergroup_metadata_map_.end(); pergroup_metadata_map_const_iter++)
        {
            // Dump the group ID
            const GroupId tmp_group_id = pergroup_metadata_map_const_iter->first;
            fs_ptr->write((const char*)&tmp_group_id, sizeof(GroupId));

            // Dump GroupLevelMetadata
            const GroupLevelMetadata& group_level_metadata = pergroup_metadata_map_const_iter->second;
            group_level_metadata.dumpGroupLevelMetadata(fs_ptr);
        }

        // Dump lookup table
        // (1) perkey_lookup_table_key_size_
        fs_ptr->write((const char*)&perkey_lookup_table_key_size_, sizeof(uint64_t));
        // NOTE: NO need to dump perkey_lookup_table_, which can be built from scratch based on object-level and group-level loaded snapshot

        return;
    }

    void LocalUncachedLru::loadLocalUncachedLru(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Data structures used to rebuild lookup table
        std::unordered_map<Key, perkey_metadata_list_iter_t, KeyHasher> perkey_metadata_listiter;

        // Clear object-level metadata (updated due to replaying adding local uncached objects into the small LRU cache)
        perkey_metadata_list_key_size_ = 0;
        perkey_metadata_list_.clear();

        // Load and update object-level metadata
        // (1) perkey_metadata_list_key_size_
        fs_ptr->read((char*)&perkey_metadata_list_key_size_, sizeof(uint64_t));
        // (2) key-KeyLevelMetadata pairs
        uint32_t perkey_metadata_list_size = 0;
        fs_ptr->read((char*)&perkey_metadata_list_size, sizeof(uint32_t));
        for (uint32_t i = 0; i < perkey_metadata_list_size; i++)
        {
            // Load the key
            Key tmp_key;
            tmp_key.deserialize(fs_ptr);

            // Load the KeyLevelMetadata
            HomoKeyLevelMetadata key_level_metadata;
            key_level_metadata.loadKeyLevelMetadata(fs_ptr);

            // Update key-level metadata list
            perkey_metadata_list_.push_back(std::pair<Key, HomoKeyLevelMetadata>(tmp_key, key_level_metadata));

            perkey_metadata_list_iter_t tmp_perkey_metadata_list_iter = perkey_metadata_list_.end();
            tmp_perkey_metadata_list_iter--;
            perkey_metadata_listiter.insert(std::pair(tmp_key, tmp_perkey_metadata_list_iter));
        }

        // Clear object-level metadata (updated due to replaying admissions)
        cur_group_id_ = 0;
        cur_group_keycnt_ = 0;
        pergroup_metadata_map_.clear();

        // Load and update object-level metadata
        // (1) cur_group_id_
        fs_ptr->read((char*)&cur_group_id_, sizeof(GroupId));
        // (2) cur_group_keycnt_
        fs_ptr->read((char*)&cur_group_keycnt_, sizeof(uint32_t));
        // (3) groupid-GroupLevelMetadata pairs
        uint32_t pergroup_metadata_map_size = 0;
        fs_ptr->read((char*)&pergroup_metadata_map_size, sizeof(uint32_t));
        for (uint32_t i = 0; i < pergroup_metadata_map_size; i++)
        {
            // Load the group ID
            GroupId tmp_group_id;
            fs_ptr->read((char*)&tmp_group_id, sizeof(GroupId));

            // Load the GroupLevelMetadata
            GroupLevelMetadata group_level_metadata;
            group_level_metadata.loadGroupLevelMetadata(fs_ptr);

            // Update group-level metadata map
            pergroup_metadata_map_.insert(std::pair<GroupId, GroupLevelMetadata>(tmp_group_id, group_level_metadata));
        }

        // Clear lookup table (updated due to replaying admissions)
        perkey_lookup_table_key_size_ = 0;
        perkey_lookup_table_.clear();

        // Load and update lookup table
        // (1) perkey_lookup_table_key_size_
        fs_ptr->read((char*)&perkey_lookup_table_key_size_, sizeof(uint64_t));
        // NOTE: NO need to load perkey_lookup_table_, which can be built from scratch based on object-level and group-level loaded snapshot
        for (typename std::unordered_map<Key, perkey_metadata_list_iter_t>::const_iterator perkey_metadata_listiter_const_iter = perkey_metadata_listiter.begin(); perkey_metadata_listiter_const_iter != perkey_metadata_listiter.end(); perkey_metadata_listiter_const_iter++)
        {
            // Get the key
            const Key& tmp_key = perkey_metadata_listiter_const_iter->first;

            // Get the perkey_metadata_list_iter
            perkey_metadata_list_iter_t tmp_perkey_metadata_list_iter = perkey_metadata_listiter_const_iter->second;
            assert(tmp_perkey_metadata_list_iter != perkey_metadata_list_.end());

            // Update lookup table
            perkey_lookup_table_.insert(std::pair(tmp_key, tmp_perkey_metadata_list_iter));
        }

        return;
    }

    void LocalUncachedLru::calculateAndUpdatePopularity_(perkey_metadata_list_t::iterator& perkey_metadata_list_iter, const HomoKeyLevelMetadata& key_level_metadata_ref, const GroupLevelMetadata& group_level_metadata_ref)
    {
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize object_size = key_level_metadata_ref.getObjectSize();
        #else
        ObjectSize objsize_size = static_cast<ObjectSize>(group_level_metadata_ref.getAvgObjectSize());
        #endif

        // Calculate and update local uncached popularity
        Frequency local_frequency = key_level_metadata_ref.getLocalFrequency();
        Popularity local_uncached_popularity = calculatePopularity(local_frequency, object_size);
        perkey_metadata_list_iter->second.updateLocalPopularity(local_uncached_popularity);

        return;
    }

    Reward LocalUncachedLru::calculateReward_(const EdgeWrapperBase* edge_wrapper_ptr, perkey_metadata_list_t::iterator perkey_metadata_list_iter) const
    {
        // Get local uncached popularity
        const Popularity local_uncached_popularity = perkey_metadata_list_iter->second.getLocalPopularity();
        const bool is_global_cached = perkey_metadata_list_iter->second.isGlobalCached();

        // Calculte local reward (i.e., min admission benefit, as the local edge node does NOT know cache miss status of all other edge nodes and conservatively treat it as a local single placement)
        const Popularity redirected_uncached_popularity = 0.0; // NOTE: local cache node does not have the view of redirected uncached popularity -> set it to 0.0
        CalcLocalUncachedRewardFuncParam tmp_param(local_uncached_popularity, is_global_cached, redirected_uncached_popularity);
        edge_wrapper_ptr->constCustomFunc(CalcLocalUncachedRewardFuncParam::FUNCNAME, &tmp_param);
        Reward local_reward = tmp_param.getRewardConstRef();

        return local_reward;
    }

    bool LocalUncachedLru::needEraseForUncachedObjects_(Key& erased_key) const
    {
        //uint32_t cur_trackcnt = perkey_lookup_table_.size();
        uint64_t cache_size_usage = getSizeForCapacity();
        if (cache_size_usage > max_bytes_for_local_uncached_lru_)
        {
            assert(perkey_metadata_list_.size() > 0);

            perkey_metadata_list_t::const_iterator tmp_const_iter = perkey_metadata_list_.end();
            tmp_const_iter--; // Point to the last element in object-level metadata LRU list
            erased_key = tmp_const_iter->first;
            return true;
        }
        else
        {
            return false;
        }
    }
}