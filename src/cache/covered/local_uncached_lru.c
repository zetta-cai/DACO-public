#include "cache/covered/local_uncached_lru.h"

#include <assert.h>

#include "common/util.h"

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
}