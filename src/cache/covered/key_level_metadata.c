#include "cache/covered/key_level_metadata.h"

#include <assert.h>

namespace covered
{
    const std::string KeyLevelMetadata::kClassName("KeyLevelMetadata");

    KeyLevelMetadata::KeyLevelMetadata(const GroupId& group_id) : group_id_(group_id)
    {
        frequency_ = 0;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
    }

    KeyLevelMetadata::KeyLevelMetadata(const KeyLevelMetadata& other) : group_id_(other.group_id_)
    {
        frequency_ = other.frequency_;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = other.object_size_;
        #endif
    }

    KeyLevelMetadata::~KeyLevelMetadata() {}

    void KeyLevelMetadata::updateNoValueDynamicMetadata()
    {
        frequency_++;

        return;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    void KeyLevelMetadata::updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size)
    {
        
        assert(object_size_ == original_object_size);
        object_size_ = object_size;

        return;
    }
    #endif

    GroupId KeyLevelMetadata::getGroupId() const
    {
        return group_id_;
    }

    Frequency KeyLevelMetadata::getFrequency() const
    {
        return frequency_;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    ObjectSize KeyLevelMetadata::getObjectSize() const
    {
        return object_size_;
    }
    #endif

    uint64_t KeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = sizeof(GroupId) + sizeof(Frequency);
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        total_size += sizeof(ObjectSize);
        #endif
        return total_size;
    }
}