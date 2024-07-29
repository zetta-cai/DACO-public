#include "cache/covered/key_level_metadata_base.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string KeyLevelMetadataBase::kClassName("KeyLevelMetadataBase");

    KeyLevelMetadataBase::KeyLevelMetadataBase() : group_id_(0)
    {
        local_frequency_ = 0;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
        local_popularity_ = 0.0;

        is_valid_ = false;
    }

    KeyLevelMetadataBase::KeyLevelMetadataBase(const GroupId& group_id, const bool& is_neighbor_cached) : group_id_(group_id)
    {
        UNUSED(is_neighbor_cached);

        local_frequency_ = 0;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = 0;
        #endif
        local_popularity_ = 0.0;

        // NOTE: is_valid_ will be set by derived classes
    }

    KeyLevelMetadataBase::KeyLevelMetadataBase(const KeyLevelMetadataBase& other) : group_id_(other.group_id_)
    {
        local_frequency_ = other.local_frequency_;
        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
        object_size_ = other.object_size_;
        #endif
        local_popularity_ = other.local_popularity_;

        is_valid_ = other.is_valid_;
    }

    KeyLevelMetadataBase::~KeyLevelMetadataBase() {}

    void KeyLevelMetadataBase::updateValueDynamicMetadata(const ObjectSize& object_size, const ObjectSize& original_object_size)
    {
        checkValidity_();

        #ifdef ENABLE_TRACK_PERKEY_OBJSIZE        
        assert(object_size_ == original_object_size);
        object_size_ = object_size;
        #endif
        
        return;
    }

    void KeyLevelMetadataBase::updateLocalPopularity(const Popularity& local_popularity)
    {
        checkValidity_();

        local_popularity_ = local_popularity;

        return;
    }

    GroupId KeyLevelMetadataBase::getGroupId() const
    {
        checkValidity_();

        return group_id_;
    }

    Frequency KeyLevelMetadataBase::getLocalFrequency() const
    {
        checkValidity_();

        return local_frequency_;
    }

    #ifdef ENABLE_TRACK_PERKEY_OBJSIZE
    ObjectSize KeyLevelMetadataBase::getObjectSize() const
    {
        checkValidity_();

        return object_size_;
    }
    #endif

    Popularity KeyLevelMetadataBase::getLocalPopularity() const
    {
        checkValidity_();

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
        checkValidity_();
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

        // NOTE: is_valid_ will be set by derived classes

        return;
    }

    void KeyLevelMetadataBase::checkValidity_() const
    {
        assert(is_valid_ == true);
        return;
    }

    void KeyLevelMetadataBase::updateNoValueDynamicMetadata_(const Frequency& added_local_frequency)
    {
        checkValidity_();

        local_frequency_ += added_local_frequency; // By default is +1

        return;
    }
}