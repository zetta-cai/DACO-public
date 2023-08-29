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

        bool needDetrackForUncachedObjects(Key& detracked_key) const; // Check if need to detrack the least popular key for local uncached object
        uint32_t getApproxValueForUncachedObjects(const Key& key) const; // Get approximated value for local uncached object

        void updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related); // Tracked uncached key (is_value_related = false: for getrsp with cache miss; is_value_related = true: put/delrsp with cache miss)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local uncached objects
    private:
        static const std::string kClassName;

        const uint64_t max_bytes_for_uncached_objects_; // Used only for local uncached objects
    };
}

#endif