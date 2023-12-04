#include "core/popularity/collected_popularity.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string CollectedPopularity::kClassName = "CollectedPopularity";

    CollectedPopularity::CollectedPopularity()
    {
        is_tracked_ = false;
        local_uncached_popularity_ = 0.0;
        object_size_ = 0;

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        with_valid_value_ = false;
        value_ = Value();
        #endif
    }

    CollectedPopularity::CollectedPopularity(const bool& is_tracked, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& with_valid_value, const Value& value)
    {
        is_tracked_ = is_tracked;

        local_uncached_popularity_ = local_uncached_popularity;
        object_size_ = object_size;

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        with_valid_value_ = with_valid_value;
        value_ = value;
        if (with_valid_value)
        {
            assert(object_size >= value.getValuesize()); // object size = key size + value size >= value size
        }
        #else
        UNUSED(with_valid_value);
        UNUSED(value);
        #endif
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

    #ifdef ENABLE_AUXILIARY_DATA_CACHE
    bool CollectedPopularity::withValidValue() const
    {
        assert(is_tracked_ == true);
        return with_valid_value_;
    }

    Value CollectedPopularity::getValue() const
    {
        assert(is_tracked_ == true);
        return value_;
    }
    #endif

    uint32_t CollectedPopularity::getCollectedPopularityPayloadSize() const
    {
        uint32_t collected_popularity_payload_size = 0;
        if (is_tracked_) // local_uncached_popularity_ and object_size_ (also with_valid_value_ and value_ if ENABLE_AUXILIARY_DATA_CACHE) are valid
        {
            // track flag + local uncached popularity + object size
            collected_popularity_payload_size = sizeof(bool) + sizeof(Popularity) + sizeof(ObjectSize);

            #ifdef ENABLE_AUXILIARY_DATA_CACHE
            collected_popularity_payload_size += sizeof(bool); // with valid value flag
            if (with_valid_value_) // value if valid
            {
                collected_popularity_payload_size += value_.getValuePayloadSize();
            }
            #endif
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
        if (is_tracked_) // local_uncached_popularity_ and object_size_ (also with_valid_value_ and value_ if ENABLE_AUXILIARY_DATA_CACHE) are valid
        {
            msg_payload.deserialize(size, (const char*)&local_uncached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
            msg_payload.deserialize(size, (const char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);

            #ifdef ENABLE_AUXILIARY_DATA_CACHE
            msg_payload.deserialize(size, (const char*)&with_valid_value_, sizeof(bool));
            size += sizeof(bool);
            if (with_valid_value_)
            {
                uint32_t value_serialize_size = value_.serialize(msg_payload, size);
                size += value_serialize_size;
            }
            #endif
        }
        return size - position;
    }

    uint32_t CollectedPopularity::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&is_tracked_, sizeof(bool));
        size += sizeof(bool);
        if (is_tracked_) // local_uncached_popularity_ and object_size_ (also with_valid_value_ and value_ if ENABLE_AUXILIARY_DATA_CACHE) are valid
        {
            msg_payload.serialize(size, (char*)&local_uncached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
            msg_payload.serialize(size, (char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);

            #ifdef ENABLE_AUXILIARY_DATA_CACHE
            msg_payload.serialize(size, (char*)&with_valid_value_, sizeof(bool));
            size += sizeof(bool);
            if (with_valid_value_)
            {
                uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
                size += value_deserialize_size;
            }
            #endif
        }
        return size - position;
    }

    const CollectedPopularity& CollectedPopularity::operator=(const CollectedPopularity& other)
    {
        is_tracked_ = other.is_tracked_;
        local_uncached_popularity_ = other.local_uncached_popularity_;
        object_size_ = other.object_size_;

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        with_valid_value_ = other.with_valid_value_;
        value_ = other.value_;
        #endif

        return *this;
    }
}