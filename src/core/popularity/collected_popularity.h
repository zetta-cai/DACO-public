/*
 * CollectedPopularity: a flag indicating whether the local uncached popularity is valid (i.e., belonging to a popular key tracked by local uncached metadata) and a local uncached popularity.
 * 
 * By Siyuan Sheng (2023.09.07).
 */

#ifndef COLLECTED_POPULARITY_H
#define COLLECTED_POPULARITY_H

#include <string>

#include "cache/covered/common_header.h"
#include "common/dynamic_array.h"

namespace covered
{
    class CollectedPopularity
    {
    public:
        CollectedPopularity();
        CollectedPopularity(const bool& is_tracked, const Popularity& local_uncached_popularity);
        ~CollectedPopularity();

        bool isTracked() const;
        Popularity getLocalUncachedPopularity() const;

        uint32_t getCollectedPopularityPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const CollectedPopularity& operator=(const CollectedPopularity& other);
    private:
        static const std::string kClassName;

        bool is_tracked_; // Whether key is tracked by the sender (i.e., whether local_uncached_popularity_ is valid)
        Popularity local_uncached_popularity_; // For popularity collection
    };
}

#endif