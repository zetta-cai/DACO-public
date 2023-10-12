#include "cache/covered/local_cached_metadata.h"

#include "common/util.h"

namespace covered
{
    const std::string LocalCachedMetadata::kClassName("LocalCachedMetadata");

    LocalCachedMetadata::LocalCachedMetadata() : CacheMetadataBase() {}

    LocalCachedMetadata::~LocalCachedMetadata() {}

    // ONLY for local cached objects

    bool LocalCachedMetadata::getLeastPopularKeyAndPopularity(const uint32_t& least_popular_rank, Key& key, Popularity& local_cached_popularity, Popularity& redirected_cached_popularity) const
    {
        bool is_least_popular_key_exist = false;

        if (least_popular_rank < sorted_popularity_multimap_.size())
        {
            sorted_popularity_multimap_t::const_iterator sorted_popularity_iter = sorted_popularity_multimap_.begin();
            std::advance(sorted_popularity_iter, least_popular_rank);

            key = sorted_popularity_iter->second;
            local_cached_popularity = sorted_popularity_iter->first;
            redirected_cached_popularity = 0.0; // TODO: update after introducing heterogeneous popularity calculation

            is_least_popular_key_exist = true;
        }

        return is_least_popular_key_exist;
    }

    // Different for local cached objects

    bool LocalCachedMetadata::addForNewKey(const Key& key, const Value& value, const uint32_t& peredge_synced_victimcnt)
    {
        CacheMetadataBase::addForNewKey_(key, value);

        bool affect_victim_tracker = false;

        // Get lookup iterator (key becomes existing after addForNewKey_)
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(key);

        uint32_t least_popular_rank_before_metadata_update = getLeastPopularRank_(perkey_lookup_iter);
        if (least_popular_rank_before_metadata_update < peredge_synced_victimcnt) // If key was a local synced victim before
        {
            affect_victim_tracker = true;
        }

        return affect_victim_tracker;
    }

    bool LocalCachedMetadata::updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related, const uint32_t& peredge_synced_victimcnt)
    {
        bool affect_victim_tracker = false;

        // Get lookup iterator
        perkey_lookup_iter_t perkey_lookup_iter = getLookup_(key);

        uint32_t least_popular_rank_before_metadata_update = getLeastPopularRank_(perkey_lookup_iter);
        if (least_popular_rank_before_metadata_update < peredge_synced_victimcnt) // If key was a local synced victim before
        {
            affect_victim_tracker = true;
        }

        CacheMetadataBase::updateForExistingKey_(key, value, original_value, is_value_related);

        if (!affect_victim_tracker)
        {
            uint32_t least_popular_rank_after_metadata_update = getLeastPopularRank_(perkey_lookup_iter);
            if (least_popular_rank_after_metadata_update < peredge_synced_victimcnt) // If key is a local synced victim now
            {
                affect_victim_tracker = true;
            }
        }

        return affect_victim_tracker;
    }

    uint64_t LocalCachedMetadata::getSizeForCapacity() const
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
        // NOTE: (i) LRU list does NOT need to track keys; (ii) although we track local cached metadata outside CacheLib to avoid extensive hacking, per-key object-level metadata actually can be stored into cachelib::CacheItem -> NO need to count the size of keys of object-level metadata for local cached objects (similar as size measurement in ValidityMap and BlockTracker)
        //total_size = Util::uint64Add(total_size, perkey_metadata_list_key_size_);
        total_size = Util::uint64Add(total_size, perkey_metadata_list_metadata_size);

        // Group-level metadata
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_groupid_size);
        total_size = Util::uint64Add(total_size, pergroup_metadata_map_metadata_size);

        // Popularity information (locate keys in lookup table for popularity-based eviction)
        // NOTE: we only need to maintain keys (or CacheItem pointers) in popularity list instead of LRU list (we NEVER use perkey_metadata_list_->first to locate keys) for popularity-based eviction, while cachelib has counted the size of keys (i.e., CacheItem pointers) in MMContainer's LRU list (we do NOT remove keys from LRU list in cachelib to avoid hacking too much code) -> NO need to count the size of keys of popularity list for local cached objects here
        total_size = Util::uint64Add(total_size, sorted_popularity_multimap_popularity_size);
        //total_size = Util::uint64Add(total_size, sorted_popularity_multimap_key_size_);

        // Lookup table (locate keys in object-level LRU list and popularity list)
        // NOTE: although we track local cached metadata outside CacheLib to avoid extensive hacking, per-key lookup metadata actually can be stored into cachelib::ChainedHashTable -> NO need to count the size of keys and object-level metadata iterators of lookup metadata for local cached objects
        //total_size = Util::uint64Add(total_size, perkey_lookup_table_key_size_);
        //total_size = Util::uint64Add(total_size, perkey_lookup_table_perkey_metadata_iter_size); // LRU list iterator
        UNUSED(perkey_lookup_table_perkey_metadata_iter_size);
        total_size = Util::uint64Add(total_size, perkey_lookup_table_sorted_popularity_iter_size); // Popularity list iterator

        return total_size;
    }
}