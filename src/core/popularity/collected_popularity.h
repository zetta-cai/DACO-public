/*
 * CollectedPopularity: use a flag to indicate whether local uncached popularity and object size are valid (i.e., belonging to a popular key tracked by local uncached metadata).
 * 
 * By Siyuan Sheng (2023.09.07).
 */

#ifndef COLLECTED_POPULARITY_H
#define COLLECTED_POPULARITY_H

#include <string>

#include "common/covered_common_header.h"
#include "common/dynamic_array.h"
#include "common/value.h"

namespace covered
{
    class CollectedPopularity
    {
    public:
        CollectedPopularity();
        CollectedPopularity(const bool& is_tracked, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& with_valid_value, const Value& value);
        ~CollectedPopularity();

        bool isTracked() const;

        // For popularity collection
        Popularity getLocalUncachedPopularity() const;

        // For placement calculation
        ObjectSize getObjectSize() const;

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // For fast cache warmup
        bool withValidValue() const;
        Value getValue() const;
        #endif

        uint32_t getCollectedPopularityPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const CollectedPopularity& operator=(const CollectedPopularity& other);
    private:
        static const std::string kClassName;

        bool is_tracked_; // Whether key is tracked by the sender (i.e., whether local_uncached_popularity_ and object_size_ is valid)

        // For popularity collection
        Popularity local_uncached_popularity_;

        // For placement calculation
        ObjectSize object_size_;

        #ifdef ENABLE_AUXILIARY_DATA_CACHE
        // For fast cache warmup
        bool with_valid_value_;
        Value value_;
        #endif
    };
}

#endif