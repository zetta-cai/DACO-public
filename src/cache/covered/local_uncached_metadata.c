#include "cache/covered/local_uncached_metadata.h"

#include "common/util.h"

namespace covered
{
    const std::string LocalUncachedMetadata::kClassName("LocalUncachedMetadata");

    LocalUncachedMetadata::LocalUncachedMetadata(const uint64_t& max_bytes_for_uncached_objects) : CacheMetadataBase(), max_bytes_for_uncached_objects_(max_bytes_for_uncached_objects)
    {
        assert(max_bytes_for_uncached_objects > 0);
    }

    LocalUncachedMetadata::~LocalUncachedMetadata() {}

    // ONLY for local uncached objects

    bool LocalUncachedMetadata::getLocalUncachedPopularityAndAvgObjectSize(const Key& key, Popularity& local_uncached_popularity, ObjectSize& avg_object_size) const
    {
        bool is_key_exist = false;

        // Get lookup iterator
        perkey_lookup_const_iter_t perkey_lookup_const_iter = tryToGetLookup_(key);

        // Get popularity if key exists
        if (perkey_lookup_const_iter != perkey_lookup_table_.end())
        {
            local_uncached_popularity = getPopularity_(perkey_lookup_const_iter);

            const GroupLevelMetadata& pergroup_metadata_ref = getGroupLevelMetadata_(perkey_lookup_const_iter);
            avg_object_size = pergroup_metadata_ref.getAvgObjectSize();

            is_key_exist = true;
        }

        return is_key_exist;
    }

    uint32_t LocalUncachedMetadata::getApproxValueSizeForUncachedObjects(const Key& key) const
    {
        // Get lookup iterator
        perkey_lookup_const_iter_t perkey_lookup_const_iter = getLookup_(key);

        // Get average object size
        const GroupLevelMetadata& pergroup_metadata_ref = getGroupLevelMetadata_(perkey_lookup_const_iter);
        uint32_t avg_object_size = pergroup_metadata_ref.getAvgObjectSize();

        // NOTE: for local uncached objects, as we do NOT know per-key value size, we use the (average object size - key size) as the approximated detrack value
        uint32_t approx_value_size = 0;
        uint32_t detrack_key_size = key.getKeyLength();
        if (avg_object_size > detrack_key_size)
        {
            approx_value_size = avg_object_size - detrack_key_size;
        }

        return approx_value_size;
    }

    bool LocalUncachedMetadata::needDetrackForUncachedObjects_(Key& detracked_key) const
    {
        //uint32_t cur_trackcnt = perkey_lookup_table_.size();
        uint64_t cache_size_usage_for_uncached_objects = getSizeForCapacity();
        if (cache_size_usage_for_uncached_objects > max_bytes_for_uncached_objects_)
        {
            detracked_key = sorted_popularity_multimap_.begin()->second;
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
        CacheMetadataBase::addForNewKey_(key, value);

        Key detracked_key;
        while (true)
        {
            bool need_detrack = needDetrackForUncachedObjects_(detracked_key);
            if (need_detrack) // Cache size usage for local uncached objects exceeds the max bytes limitation
            {
                uint32_t approx_detracked_value_size = getApproxValueSizeForUncachedObjects(detracked_key);
                removeForExistingKey(detracked_key, Value(approx_detracked_value_size)); // For getrsp with cache miss, put/delrsp with cache miss
            }
            else // Local uncached objects is limited
            {
                break;
            }
        }

        return;
    }

    void LocalUncachedMetadata::updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related)
    {
        CacheMetadataBase::updateForExistingKey_(key, value, original_value, is_value_related);

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
        // NOTE: although we maintain keys in perkey_metadata_list_, it is just for implementation simplicity but we NEVER use perkey_metadata_list_->first to locate keys (due to popularity-based eviciton instead of LRU-based eviction) -> NO need to count the size of keys of object-level metadata for local uncached objects (cache size usage of keys will be counted in lookup table below)
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

        return total_size;
    }
}