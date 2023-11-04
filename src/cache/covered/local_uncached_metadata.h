/*
 * LocalUncachedMetadata: metadata for local cached objects in covered.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_UNCACHED_METADATA_H
#define LOCAL_UNCACHED_METADATA_H

#include <string>

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
        bool getLocalUncachedObjsizePopularityForKey(const Key& key, ObjectSize& object_size, Popularity& local_uncached_popularity) const; // Get popularity and average object size for local uncached object; return true if key exists (i.e., tracked)

        uint32_t getValueSizeForUncachedObjects(const Key& key) const; // Get accurate/approximate value size for local uncached object

        // Different for local uncached objects

        void addForNewKey(const Key& key, const Value& value); // Currently tracked uncached key (for getrsp with cache miss, put/delrsp with cache miss); overwrite CacheMetadataBase to detrack old keys if necessary

        void updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related); // Tracked uncached key (is_value_related = false: for getrsp with cache miss; is_value_related = true: put/delrsp with cache miss); NOT overwrite CacheMetadataBase

        void removeForExistingKey(const Key& detracked_key, const Value& value); // Remove tracked uncached key (for getrsp with cache miss, put/delrsp with cache miss, admission)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local uncached objects
    private:
        static const std::string kClassName;

        // ONLY for local uncached objects

        bool needDetrackForUncachedObjects_(Key& detracked_key) const; // Check if need to detrack the least popular key for local uncached object

        const uint64_t max_bytes_for_uncached_objects_; // Used only for local uncached objects
    };
}

#endif