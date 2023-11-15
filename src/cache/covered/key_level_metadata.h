/*
 * KeyLevelMetadata: key-level metadata for each object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef KEY_LEVEL_METADATA_H
#define KEY_LEVEL_METADATA_H

#include <string>

#include "common/covered_common_header.h"

namespace covered
{
    class KeyLevelMetadata
    {
    public:
        KeyLevelMetadata(const GroupId& group_id);
        KeyLevelMetadata(const KeyLevelMetadata& other);
        ~KeyLevelMetadata();

        void updateNoValueDynamicMetadata(); // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        void updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size); // For admission, put/delreq w/ hit/miss, and getrsp w/ invalid-hit (also getrsp w/ miss if for newly-tracked key), update object-level value-related metadata
        #endif
        void updateLocalPopularity(const Popularity& local_popularity);

        GroupId getGroupId() const;
        Frequency getFrequency() const;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        ObjectSize getObjectSize() const;
        #endif
        Popularity getLocalPopularity() const; // Get local popularity for local requests (local hits for local cached objects or local misses for local uncached objects)

        static uint64_t getSizeForCapacity();
    private:
        static const std::string kClassName;

        // Const metadata
        const GroupId group_id_;

        // Non-const value-unrelated dynamic metadata
        Frequency frequency_;

        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        // Non-const value-related dynamic metadata
        ObjectSize object_size_;
        #endif

        // Popularity information
        Popularity local_popularity_; // Popularity for local requests (local hits for local cached objects or local misses for local uncached objects)
    };
}

#endif