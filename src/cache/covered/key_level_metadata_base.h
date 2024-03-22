/*
 * KeyLevelMetadataBase: base class with common key-level metadata for homogeneous/heterogeneous key-level metadata of each local uncached/cached object in local edge cache.
 * 
 * By Siyuan Sheng (2023.11.18).
 */

#ifndef KEY_LEVEL_METADATA_BASE_H
#define KEY_LEVEL_METADATA_BASE_H

#include <string>

#include "common/covered_common_header.h"

namespace covered
{
    class KeyLevelMetadataBase
    {
    public:
        KeyLevelMetadataBase(const GroupId& group_id, const bool& is_neighbor_cached); // NOTE: is_neighbor_cached ONLY used by HeteroKeyLevelMetadata for local cached metadata
        KeyLevelMetadataBase(const KeyLevelMetadataBase& other);
        ~KeyLevelMetadataBase();

        virtual void updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached) = 0; // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata (is_redirected ONLY used by local cached metadata yet always false for local uncached metadata; is_global_cached ONLY used by local uncached metadata yet always true for local cached metadata)

        void updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size); // For admission, put/delreq w/ hit/miss, and getrsp w/ invalid-hit (also getrsp w/ miss if for newly-tracked key), update object-level value-related metadata
        void updateLocalPopularity(const Popularity& local_popularity);

        GroupId getGroupId() const;
        Frequency getLocalFrequency() const;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize getObjectSize() const;
        #endif
        Popularity getLocalPopularity() const; // Get local popularity for local requests (local hits for local cached objects or local misses for local uncached objects)

        static uint64_t getSizeForCapacity();

        // Dump/load key-level metadata for local cached/uncached metadata of cache metadata in cache snapshot
        virtual void dumpKeyLevelMetadata(std::fstream* fs_ptr) const;
        virtual void loadKeyLevelMetadata(std::fstream* fs_ptr);
    protected:
        void updateNoValueDynamicMetadata_(); // Update value-unrelated dynamic metadata
    private:
        static const std::string kClassName;

        // Const metadata
        GroupId group_id_;

        // Non-const value-unrelated dynamic metadata
        Frequency local_frequency_;

        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        // Non-const value-related dynamic metadata
        ObjectSize object_size_;
        #endif

        // Local popularity information
        Popularity local_popularity_; // Local popularity for local requests (local hits for local cached objects or local misses for local uncached objects)
    };
}

#endif