/*
 * HomoKeyLevelMetadata: homogeneous key-level metadata for each local uncached object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef HOMO_KEY_LEVEL_METADATA_H
#define HOMO_KEY_LEVEL_METADATA_H

#include <string>

#include "common/covered_common_header.h"

namespace covered
{
    class HomoKeyLevelMetadata
    {
    public:
        HomoKeyLevelMetadata(const GroupId& group_id);
        HomoKeyLevelMetadata(const HomoKeyLevelMetadata& other);
        ~HomoKeyLevelMetadata();

        void updateNoValueDynamicMetadata(const bool& is_redirected = false); // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        void updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size); // For admission, put/delreq w/ hit/miss, and getrsp w/ invalid-hit (also getrsp w/ miss if for newly-tracked key), update object-level value-related metadata
        #endif
        void updateLocalPopularity(const Popularity& local_popularity);

        GroupId getGroupId() const;
        Frequency getLocalFrequency() const;
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