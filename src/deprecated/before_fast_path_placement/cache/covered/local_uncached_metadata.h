/*
 * LocalUncachedMetadata: metadata for local cached objects in covered.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_UNCACHED_METADATA_H
#define LOCAL_UNCACHED_METADATA_H

#include <string>
#include <unordered_map>

#include "cache/covered/cache_metadata_base.h"

namespace covered
{
    class LocalUncachedMetadata : public CacheMetadataBase
    {
    public:
        LocalUncachedMetadata(const uint64_t& max_bytes_for_uncached_objects);
        virtual ~LocalUncachedMetadata();

        // ONLY for local uncached objects

        // TODO: Track key-level object size instead of group-level one in CacheMetadataBase for eviction cost in placement calculation if necessary
        bool getLocalUncachedObjsizePopularityValueForKey(const Key& key, ObjectSize& object_size, Popularity& local_uncached_popularity, bool& with_valid_value, Value& value) const; // Get popularity and average object size (and value if ENABLE_AUXILIARY_DATA_CACHE) for local uncached object; return true if key exists (i.e., tracked)

        uint32_t getValueSizeForUncachedObjects(const Key& key) const; // Get accurate/approximate value size for local uncached object

        // Different for local uncached objects

        void addForNewKey(const Key& key, const Value& value); // For getrsp/put/delreq w/ miss (also getreq w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP for newly-tracked key), initialize and update both value-unrelated and value-related metadata for newly-tracked key

        void updateNoValueStatsForExistingKey(const Key& key); // For get/put/delreq w/ miss, update object-level value-unrelated metadata for existing key (i.e., already tracked objects for local uncached)
        void updateValueStatsForExistingKey(const Key& key, const Value& value, const Value& original_value); // For put/delreq w/ miss (also getrsp w/ miss if ENABLE_CONSERVATIVE_UNCACHED_POP for existing key), update object-level value-related metadata for existing key (i.e., already tracked objects for local uncached)

        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove tracked uncached key (for getrsp/put/delreq with cache miss and admission)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local uncached objects
    private:
        static const std::string kClassName;

        // ONLY for local uncached objects

        bool needDetrackForUncachedObjects_(Key& detracked_key) const; // Check if need to detrack the least popular key for local uncached object

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // NOTE: maintain auxiliary data cache outside CacheMetadataBase is just for implementation simplicity (e.g., we can track the value in KeyLevelMetadata at CacheMetadataBase if the key is local uncached), while we NEVER use auxiliary_data_cache_ to lookup keys for cache metadata of local uncached objects, so we ONLY count the space usage of values instead of keys for auxiliary_data_cache_ here
        std::unordered_map<Key, Value, KeyHasher> auxiliary_data_cache_; // Auxiliary data cache for local uncached objects
        #endif

        const uint64_t max_bytes_for_uncached_objects_; // Used only for local uncached objects
    };
}

#endif