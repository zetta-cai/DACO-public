/*
 * LocalUncachedLru: a small LRU cache of access statistics (including object-level, group-level and sorted popularity metadata) for recently-accessed locally-uncached objects (yet not tracked by LocalUncachedMetadata).
 *
 * NOTE: object-level metadata and group-level metadata in LocalUncachedLru are updated for local misses for local uncached objects (refer to CacheMetadataBase (src/cache/covered/cache_metadata_base.h) for object-level LRU list and group-level metadata).
 * 
 * By Siyuan Sheng (2024.07.28).
 */

#ifndef LOCAL_UNCACHED_LRU_H
#define LOCAL_UNCACHED_LRU_H

#include <list> // std::list
#include <string>
#include <unordered_map> // std::unordered_map

#include "cache/covered/group_level_metadata.h"
#include "cache/covered/homo_key_level_metadata.h"
#include "common/covered_common_header.h"
#include "common/key.h"
#include "common/value.h"
#include "edge/edge_wrapper_base.h"

namespace covered
{
    class LocalUncachedLru
    {
    public:
        typedef std::list<std::pair<Key, HomoKeyLevelMetadata>> perkey_metadata_list_t; // LRU list of homogeneous object-level metadata
        typedef typename perkey_metadata_list_t::iterator perkey_metadata_list_iter_t;
        typedef std::unordered_map<GroupId, GroupLevelMetadata> pergroup_metadata_map_t;

        typedef std::unordered_map<Key, perkey_metadata_list_iter_t, KeyHasher> perkey_lookup_table_t;
        typedef typename perkey_lookup_table_t::iterator perkey_lookup_table_iter_t;
        typedef typename perkey_lookup_table_t::const_iterator perkey_lookup_table_const_iter_t;

        LocalUncachedLru(const uint64_t& max_bytes_for_local_uncached_lru);
        ~LocalUncachedLru();

        uint64_t getSizeForCapacity() const; // Get size for capacity constraint for local uncached objects in the small LRU cache (not tracked by local uncached metadata yet)

        // Dump/load local uncached lru for cache metadata in cache snapshot
        void dumpLocalUncachedLru(std::fstream* fs_ptr) const;
        void loadLocalUncachedLru(std::fstream* fs_ptr);
    private:
        static const std::string kClassName;

        const uint64_t max_bytes_for_local_uncached_lru_; // Used only for local uncached objects in the small LRU cache
        
        // Object-level metadata for local misses of local uncached objects
        uint64_t perkey_metadata_list_key_size_; // Total size of keys in perkey_metadata_list_
        perkey_metadata_list_t perkey_metadata_list_; // LRU list for object-level metadata (list index is recency information; descending order of recency)

        // Group-level metadata for local misses of local uncached objects
        GroupId cur_group_id_;
        uint32_t cur_group_keycnt_;
        pergroup_metadata_map_t pergroup_metadata_map_; // Group-level metadata (grouping based on the time added in the small LRU cache)

        // Lookup table
        uint64_t perkey_lookup_table_key_size_; // Total size of keys in perkey_lookup_table_
        perkey_lookup_table_t perkey_lookup_table_;
    };
}

#endif