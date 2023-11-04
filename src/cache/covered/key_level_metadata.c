#include "cache/covered/key_level_metadata.h"

namespace covered
{
    const std::string KeyLevelMetadata::kClassName("KeyLevelMetadata");

    KeyLevelMetadata::KeyLevelMetadata(const GroupId& group_id) : group_id_(group_id)
    {
        frequency_ = 0;
        #ifdef TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
    }

    KeyLevelMetadata::KeyLevelMetadata(const KeyLevelMetadata& other) : group_id_(other.group_id_)
    {
        frequency_ = other.frequency_;
        #ifdef TRACK_PERKEY_OBJSIZE
        object_size_ = other.object_size_;
        #endif
    }

    KeyLevelMetadata::~KeyLevelMetadata() {}

    void KeyLevelMetadata::updateDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size, const bool& is_value_related)
    {
        frequency_++;
        #ifdef TRACK_PERKEY_OBJSIZE
        assert(object_size_ == original_object_size);
        if (is_value_related)
        {
            object_size_ = object_size;
        }
        else
        {
            assert(object_size_ == object_size);
        }
        #endif
        return;
    }

    GroupId KeyLevelMetadata::getGroupId() const
    {
        return group_id_;
    }

    Frequency KeyLevelMetadata::getFrequency() const
    {
        return frequency_;
    }

    #ifdef TRACK_PERKEY_OBJSIZE
    ObjectSize KeyLevelMetadata::getObjectSize() const
    {
        return object_size_;
    }
    #endif

    uint64_t KeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = sizeof(GroupId) + sizeof(Frequency);
        #ifdef TRACK_PERKEY_OBJSIZE
        total_size += sizeof(ObjectSize);
        #endif
        return total_size;
    }
}