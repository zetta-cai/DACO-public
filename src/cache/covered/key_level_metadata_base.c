#include "cache/covered/key_level_metadata_base.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string KeyLevelMetadataBase::kClassName("KeyLevelMetadataBase");

    KeyLevelMetadataBase::KeyLevelMetadataBase(const GroupId& group_id, const bool& is_neighbor_cached) : group_id_(group_id)
    {
        UNUSED(is_neighbor_cached);

        local_frequency_ = 0;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
        local_popularity_ = 0.0;
    }

    KeyLevelMetadataBase::KeyLevelMetadataBase(const KeyLevelMetadataBase& other) : group_id_(other.group_id_)
    {
        local_frequency_ = other.local_frequency_;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = other.object_size_;
        #endif
        local_popularity_ = other.local_popularity_;
    }

    KeyLevelMetadataBase::~KeyLevelMetadataBase() {}

    void KeyLevelMetadataBase::updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size)
    {
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE        
        assert(object_size_ == original_object_size);
        object_size_ = object_size;
        #endif
        
        return;
    }

    void KeyLevelMetadataBase::updateLocalPopularity(const Popularity& local_popularity)
    {
        local_popularity_ = local_popularity;

        return;
    }

    GroupId KeyLevelMetadataBase::getGroupId() const
    {
        return group_id_;
    }

    Frequency KeyLevelMetadataBase::getLocalFrequency() const
    {
        return local_frequency_;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    ObjectSize KeyLevelMetadataBase::getObjectSize() const
    {
        return object_size_;
    }
    #endif

    Popularity KeyLevelMetadataBase::getLocalPopularity() const
    {
        return local_popularity_;
    }

    uint64_t KeyLevelMetadataBase::getSizeForCapacity()
    {
        uint64_t total_size = sizeof(GroupId) + sizeof(Frequency);
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        total_size += sizeof(ObjectSize);
        #endif
        total_size += sizeof(Popularity);
        return total_size;
    }

    // Dump/load key-level metadata for local cached/uncached metadata of cache metadata in cache snapshot

    void KeyLevelMetadataBase::dumpKeyLevelMetadata(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump group id
        fs_ptr->write((const char*)&group_id_, sizeof(GroupId));

        // Dump local frequency
        fs_ptr->write((const char*)&local_frequency_, sizeof(Frequency));

        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        // Dump object size
        fs_ptr->write((const char*)&object_size_, sizeof(ObjectSize));
        #endif

        // Dump local popularity
        fs_ptr->write((const char*)&local_popularity_, sizeof(Popularity));

        return;
    }

    void KeyLevelMetadataBase::loadKeyLevelMetadata(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load group id
        fs_ptr->read((char*)&group_id_, sizeof(GroupId));

        // Load local frequency
        fs_ptr->read((char*)&local_frequency_, sizeof(Frequency));

        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        // Load object size
        fs_ptr->read((char*)&object_size_, sizeof(ObjectSize));
        #endif

        // Load local popularity
        fs_ptr->read((char*)&local_popularity_, sizeof(Popularity));

        return;
    }

    void KeyLevelMetadataBase::updateNoValueDynamicMetadata_()
    {
        local_frequency_++;

        return;
    }
}