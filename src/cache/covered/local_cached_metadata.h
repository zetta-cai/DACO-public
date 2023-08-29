/*
 * LocalCachedMetadata: metadata for local cached objects in covered.
 * 
 * By Siyuan Sheng (2023.08.29).
 */

#ifndef LOCAL_CACHED_METADATA_H
#define LOCAL_CACHED_METADATA_H

#include <string>

#include "cache/covered/cache_metadata_base.h"

namespace covered
{
    class LocalCachedMetadata : public CacheMetadataBase
    {
    public:
        LocalCachedMetadata();
        virtual ~LocalCachedMetadata();

        // Return if affect local synced victims in victim tracker
        bool updateForExistingKey(const Key& key, const Value& value, const Value& original_value, const bool& is_value_related, const uint32_t& peredge_synced_victimcnt); // Admitted cached key (is_value_related = false: for getreq with cache hit; is_value_related = true: for getrsp with invalid hit, put/delreq with cache hit)

        virtual uint64_t getSizeForCapacity() const override; // Get size for capacity constraint of local cached objects
    private:
        static const std::string kClassName;
    };
}

#endif