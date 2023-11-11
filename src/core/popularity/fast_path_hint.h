/*
 * FastPathHint: hint information provided by beacon edge node to sender edge node for possible fast-path single-placement calculation.
 *
 * By Siyuan Sheng (2023.11.11).
 */

#ifndef FAST_PATH_HINT_H
#define FAST_PATH_HINT_H

#include <string>

#include "common/covered_common_header.h"
#include "common/dynamic_array.h"

namespace covered
{
    class FastPathHint
    {
    public:
        FastPathHint();
        FastPathHint(const Popularity& sum_local_uncached_popularity, const DeltaReward& smallest_max_admission_benefit);
        ~FastPathHint();

        bool isValid() const;
        const Popularity getSumLocalUncachedPopularity() const;
        const DeltaReward getSmallestMaxAdmissionBenefit() const;

        uint32_t getFastPathHintPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const FastPathHint& operator=(const FastPathHint& other);
    private:
        static const std::string kClassName;

        //NOTE: is_valid_ is ONLY used to indicate the validity of fast-path hints in FastPathHint, which decides the msgtype of directory lookup response yet NOT embedded into any message
        bool is_valid_;

        // Fast-path hints
        Popularity sum_local_uncached_popularity_; // NOTE: sum_local_uncached_popularity_ MUST NOT include local uncached popularity of sender edge node
        DeltaReward smallest_max_admission_benefit_; // NOTE: the smallest max_admission_benefit_ == MIN_ADMISSION_BENEFIT (i.e., 0.0) means that beacon has sufficient space for popularity aggregation and the current object MUST be global popular to trigger fast-path placement calculation
    };
}

#endif