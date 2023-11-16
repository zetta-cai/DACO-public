#include "cache/covered/homo_key_level_metadata.h"

#include <assert.h>

namespace covered
{
    const std::string HomoKeyLevelMetadata::kClassName("HomoKeyLevelMetadata");

    HomoKeyLevelMetadata::HomoKeyLevelMetadata(const GroupId& group_id) : group_id_(group_id)
    {
        local_frequency_ = 0;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
        local_popularity_ = 0.0;
    }

    HomoKeyLevelMetadata::HomoKeyLevelMetadata(const HomoKeyLevelMetadata& other) : group_id_(other.group_id_)
    {
        local_frequency_ = other.local_frequency_;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = other.object_size_;
        #endif
        local_popularity_ = other.local_popularity_;
    }

    HomoKeyLevelMetadata::~HomoKeyLevelMetadata() {}

    void HomoKeyLevelMetadata::updateNoValueDynamicMetadata(const bool& is_redirected)
    {
        assert(!is_redirected); // MUST be local request

        local_frequency_++;

        return;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    void HomoKeyLevelMetadata::updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size)
    {
        
        assert(object_size_ == original_object_size);
        object_size_ = object_size;

        return;
    }
    #endif

    void HomoKeyLevelMetadata::updateLocalPopularity(const Popularity& local_popularity)
    {
        local_popularity_ = local_popularity;

        return;
    }

    GroupId HomoKeyLevelMetadata::getGroupId() const
    {
        return group_id_;
    }

    Frequency HomoKeyLevelMetadata::getLocalFrequency() const
    {
        return local_frequency_;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    ObjectSize HomoKeyLevelMetadata::getObjectSize() const
    {
        return object_size_;
    }
    #endif

    Popularity HomoKeyLevelMetadata::getLocalPopularity() const
    {
        return local_popularity_;
    }

    uint64_t HomoKeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = sizeof(GroupId) + sizeof(Frequency);
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        total_size += sizeof(ObjectSize);
        #endif
        total_size += sizeof(Popularity);
        return total_size;
    }
}