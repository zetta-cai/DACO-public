#include "core/popularity/collected_popularity.h"

#include <assert.h>

namespace covered
{
    const std::string CollectedPopularity::kClassName = "CollectedPopularity";

    CollectedPopularity::CollectedPopularity()
    {
        is_tracked_ = false;
        local_uncached_popularity_ = 0.0;
        object_size_ = 0;
    }

    CollectedPopularity::CollectedPopularity(const bool& is_tracked, const Popularity& local_uncached_popularity, const ObjectSize& object_size)
    {
        is_tracked_ = is_tracked;

        local_uncached_popularity_ = local_uncached_popularity;
        object_size_ = object_size;
    }

    CollectedPopularity::~CollectedPopularity() {}

    bool CollectedPopularity::isTracked() const
    {
        return is_tracked_;
    }

    Popularity CollectedPopularity::getLocalUncachedPopularity() const
    {
        assert(is_tracked_ == true);
        return local_uncached_popularity_;
    }

    ObjectSize CollectedPopularity::getObjectSize() const
    {
        assert(is_tracked_ == true);
        return object_size_;
    }

    uint32_t CollectedPopularity::getCollectedPopularityPayloadSize() const
    {
        uint32_t collected_popularity_payload_size = 0;
        if (is_tracked_) // local_uncached_popularity_ and object_size_ are valid
        {
            // track flag + local uncached popularity + object size
            collected_popularity_payload_size = sizeof(bool) + sizeof(Popularity) + sizeof(ObjectSize);
        }
        else // local_uncached_popularity_ is invalid
        {
            // track flag
            collected_popularity_payload_size = sizeof(bool);
        }
        return collected_popularity_payload_size;
    }

    uint32_t CollectedPopularity::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&is_tracked_, sizeof(bool));
        size += sizeof(bool);
        if (is_tracked_) // local_uncached_popularity_ and object_size_ are valid
        {
            msg_payload.deserialize(size, (const char*)&local_uncached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
            msg_payload.deserialize(size, (const char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);
        }
        return size - position;
    }

    uint32_t CollectedPopularity::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&is_tracked_, sizeof(bool));
        size += sizeof(bool);
        if (is_tracked_) // local_uncached_popularity_ and object_size_ are valid
        {
            msg_payload.serialize(size, (char*)&local_uncached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
            msg_payload.serialize(size, (char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);
        }
        return size - position;
    }

    const CollectedPopularity& CollectedPopularity::operator=(const CollectedPopularity& other)
    {
        is_tracked_ = other.is_tracked_;
        local_uncached_popularity_ = other.local_uncached_popularity_;
        object_size_ = other.object_size_;

        return *this;
    }
}