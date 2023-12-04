#include "cache/covered/local_uncached_metadata.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string LocalUncachedMetadata::kClassName("LocalUncachedMetadata");

    LocalUncachedMetadata::LocalUncachedMetadata(const uint64_t& max_bytes_for_uncached_objects) : CacheMetadataBase(), max_bytes_for_uncached_objects_(max_bytes_for_uncached_objects)
    {
        assert(max_bytes_for_uncached_objects > 0);

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        auxiliary_data_cache_.clear();
        #endif
    }

    LocalUncachedMetadata::~LocalUncachedMetadata() {}

    // ONLY for local uncached objects

    bool LocalUncachedMetadata::getLocalUncachedObjsizePopularityValueForKey(const Key& key, ObjectSize& object_size, Popularity& local_uncached_popularity, bool& with_valid_value, Value& value) const
    {
        bool is_key_exist = false;

        // Get lookup iterator
        perkey_lookup_const_iter_t perkey_lookup_const_iter = tryToGetLookup_(key);

        // Get popularity if key exists
        if (perkey_lookup_const_iter != perkey_lookup_table_.end())
        {
            local_uncached_popularity = getPopularity_(perkey_lookup_const_iter);
            object_size = getObjectSize_(perkey_lookup_const_iter);

            #ifdef ENABLE_AUXILIARY_DATA_CACHE
            std::unordered_map<Key, Value, KeyHasher>::const_iterator auxiliary_data_cache_const_iter = auxiliary_data_cache_.find(key);
            // NOTE: value could NOT exist in auxiliary data cache even if key has been tracked by local uncached metadata, as we will remove value from auxiliary data cache if <with writes under MSI protocol or object size is too large for slab-based memory management> on local uncached objects tracked by local uncached metadata
            if (auxiliary_data_cache_const_iter != auxiliary_data_cache_.end())
            {
                with_valid_value = true;
                value = auxiliary_data_cache_const_iter->second;
            }
            else
            {
                std::ostringstream oss;
                oss << "value does NOT exist for key " << key.getKeystr() << " in auxiliary data cache, which may be removed by writes under MSI protocol or too-large object size under slab-based memory management";
                Util::dumpInfoMsg(kClassName, oss.str());

                with_valid_value = false;
            }
            #else
            UNUSED(with_valid_value);
            UNUSED(value);
            #endif

            is_key_exist = true;
        }

        return is_key_exist;
    }

    uint32_t LocalUncachedMetadata::getValueSizeForUncachedObjects(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_const_iter_t perkey_lookup_const_iter = getLookup_(key);

        // Get accurate/average object size
        ObjectSize object_size = getObjectSize_(perkey_lookup_const_iter);

        // NOTE: for local uncached objects, as we do NOT know per-key value size (ifndef ENABLE_TRACK_PERKEY_OBJSIZE), we use the (average object size - key size) as the approximate detrack value
        uint32_t value_size = 0;
        uint32_t key_size = key.getKeyLength();
        if (object_size > key_size)
        {
            value_size = object_size - key_size;
        }

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        // NOTE: value size from CacheMetadataBase should equal with that from auxiliary data cache if any
        std::unordered_map<Key, Value, KeyHasher>::const_iterator auxiliary_data_cache_const_iter = auxiliary_data_cache_.find(key);
        if (auxiliary_data_cache_const_iter != auxiliary_data_cache_.end())
        {
            assert(value_size == auxiliary_data_cache_const_iter->second.getValuesize());
        }
        #endif
        #endif

        return value_size;
    }

    bool LocalUncachedMetadata::needDetrackForUncachedObjects_(Key& detracked_key) const
    {
        //uint32_t cur_trackcnt = perkey_lookup_table_.size();
        uint64_t cache_size_usage_for_uncached_objects = getSizeForCapacity();
        if (cache_size_usage_for_uncached_objects > max_bytes_for_uncached_objects_)
        {
            Key tmp_key;
            Popularity tmp_popularity = 0.0;
            bool is_least_popular_key_exist = CacheMetadataBase::getLeastPopularKeyPopularity_(0, tmp_key, tmp_popularity);
            assert(is_least_popular_key_exist == true); // NOTE: the least popular uncached object MUST exist

            detracked_key = tmp_key;
            UNUSED(tmp_popularity);

            return true;
        }
        else
        {
            return false;
        }
    }

    // Different for local uncached objects

    void LocalUncachedMetadata::addForNewKey(const Key& key, const Value& value)
    {
        // Initialize and update both value-unrelated and value-related metadata for newly-tracked key
        CacheMetadataBase::addForNewKey_(key, value);

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // Add/update value into auxiliary data cache
        std::unordered_map<Key, Value, KeyHasher>::iterator auxiliary_data_cache_iter = auxiliary_data_cache_.find(key);
        if (auxiliary_data_cache_iter == auxiliary_data_cache_.end())
        {
            auxiliary_data_cache_iter = auxiliary_data_cache_.insert(std::pair(key, value)).first;
        }
        else
        {
            auxiliary_data_cache_iter->second = value;
        }
        assert(auxiliary_data_cache_iter != auxiliary_data_cache_.end());
        #endif

        // Detrack key for uncached capacity limitation if necessary
        Key detracked_key;
        while (true)
        {
            bool need_detrack = needDetrackForUncachedObjects_(detracked_key);
            if (need_detrack) // Cache size usage for local uncached objects exceeds the max bytes limitation
            {
                uint32_t detracked_value_size = getValueSizeForUncachedObjects(detracked_key);                
                removeForExistingKey(detracked_key, Value(detracked_value_size)); // For getrsp with cache miss, put/delrsp with cache miss (NOTE: this will remove value from auxiliary data cache if any)
            }
            else // Local uncached objects is limited
            {
                break;
            }
        }

        return;
    }

    void LocalUncachedMetadata::updateNoValueStatsForExistingKey(const Key& key)
    {
        // Update value-unrelated metadata
        CacheMetadataBase::updateNoValueStatsForExistingKey_(key);

        return;
    }

    void LocalUncachedMetadata::updateValueStatsForExistingKey(const Key& key, const Value& value, const Value& original_value)
    {
        // Update value-related metadata
        CacheMetadataBase::updateValueStatsForExistingKey_(key, value, original_value);

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // Add/update value into auxiliary data cache
        std::unordered_map<Key, Value, KeyHasher>::iterator auxiliary_data_cache_iter = auxiliary_data_cache_.find(key);
        if (auxiliary_data_cache_iter == auxiliary_data_cache_.end())
        {
            auxiliary_data_cache_iter = auxiliary_data_cache_.insert(std::pair(key, value)).first;
        }
        else
        {
            assert(auxiliary_data_cache_iter->second.getValuesize() == original_value.getValuesize());
            auxiliary_data_cache_iter->second = value;
        }
        assert(auxiliary_data_cache_iter != auxiliary_data_cache_.end());
        #endif

        return;
    }

    void LocalUncachedMetadata::removeForExistingKey(const Key& detracked_key, const Value& value)
    {
        const bool is_local_cached_metadata = false;
        CacheMetadataBase::removeForExistingKey_(detracked_key, value, is_local_cached_metadata);

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // Remove value from auxiliary data cache if any (for <too large object size or admission> on local uncached objects tracked by local uncached metadata, yet object-/group-level metadata in CacheMetadataBase still exist)
        std::unordered_map<Key, Value, KeyHasher>::const_iterator auxiliary_data_cache_const_iter = auxiliary_data_cache_.find(detracked_key);
        if (auxiliary_data_cache_const_iter != auxiliary_data_cache_.end())
        {
            auxiliary_data_cache_.erase(auxiliary_data_cache_const_iter);
        }
        #endif

        return;
    }

    uint64_t LocalUncachedMetadata::getSizeForCapacity() const
    {
        // Object-level metadata
        uint64_t perkey_metadata_list_metadata_size = perkey_metadata_list_.size() * KeyLevelMetadata::getSizeForCapacity();

        // Group-level metadata
        uint64_t pergroup_metadata_map_groupid_size = pergroup_metadata_map_.size() * sizeof(GroupId);
        uint64_t pergroup_metadata_map_metadata_size = pergroup_metadata_map_.size() * GroupLevelMetadata::getSizeForCapacity();

        // Popularity information
        uint64_t sorted_popularity_multimap_popularity_size = sorted_popularity_multimap_.size() * sizeof(Popularity);

        // Lookup table
        uint64_t perkey_lookup_table_perkey_metadata_iter_size = perkey_lookup_table_.size() * LookupMetadata::getPerkeyMetadataIterSizeForCapacity();
        uint64_t perkey_lookup_table_sorted_popularity_iter_size = perkey_lookup_table_.size() * LookupMetadata::getSortedPopularityIterSizeForCapacity();

        uint64_t total_size = 0;

        // Object-level metadata
        // NOTE: although we maintain keys in perkey_metadata_list_, it is just for implementation simplicity (e.g., key can be replaced by a pointer referring to the corresponding lookup table entry) but we NEVER use perkey_metadata_list_->first to locate keys (due to popularity-based eviciton instead of LRU-based eviction) -> NO need to count the size of keys of object-level metadata for local uncached objects (cache size usage of keys will be counted in lookup table below)
        //total_size = Util::uint64Add(total_size, perkey_metadata_list_key_size_);
        total_size = Util::uint64Add(total_size, perkey_metadata_list_metadata_size);

        // Group-level metadata
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_groupid_size);
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_metadata_size);

        // Popularity information (locate keys in lookup table for popularity-based eviction)
        total_size = Util::uint64Add(total_size, sorted_popularity_multimap_popularity_size);
        total_size = Util::uint64Add(total_size, sorted_popularity_multimap_key_size_);

        // Lookup table (locate keys in object-level LRU list and popularity list)
        total_size = Util::uint64Add(total_size, perkey_lookup_table_key_size_);
        total_size = Util::uint64Add(total_size, perkey_lookup_table_perkey_metadata_iter_size); // LRU list iterator
        total_size = Util::uint64Add(total_size, perkey_lookup_table_sorted_popularity_iter_size); // Popularity list iterator

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // Auxiliary data cache
        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator auxiliary_data_cache_const_iter = auxiliary_data_cache_.begin(); auxiliary_data_cache_const_iter != auxiliary_data_cache_.end(); auxiliary_data_cache_const_iter++)
        {
            // NOTE: NO need to count key bytes in auxiliary data cache due to just an impl trick, which has been counted by CacheMetadataBase
            //total_size = Util::uint64Add(total_size, auxiliary_data_cache_const_iter->first.getKeyLength());
            total_size = Util::uint64Add(total_size, auxiliary_data_cache_const_iter->second.getValuesize());
        }
        #endif

        return total_size;
    }
}