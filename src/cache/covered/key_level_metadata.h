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

        void updateDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size, const bool& is_value_related);

        GroupId getGroupId() const;
        Frequency getFrequency() const;
        #ifdef TRACK_PERKEY_OBJSIZE
        ObjectSize getObjectSize() const;
        #endif

        static uint64_t getSizeForCapacity();
    private:
        static const std::string kClassName;

        // Const metadata
        const GroupId group_id_;

        // Non-const dynamic metadata
        Frequency frequency_;
        #ifdef TRACK_PERKEY_OBJSIZE
        ObjectSize object_size_;
        #endif
    };
}

#endif