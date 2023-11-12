#include "core/popularity/fast_path_hint.h"

#include <assert.h>

namespace covered
{
    const std::string FastPathHint::kClassName("FastPathHint");

    FastPathHint::FastPathHint() : is_valid_(false), sum_local_uncached_popularity_(0.0), smallest_max_admission_benefit_(MIN_ADMISSION_BENEFIT)
    {
    }

    FastPathHint::FastPathHint(const Popularity& sum_local_uncached_popularity, const DeltaReward& smallest_max_admission_benefit) : is_valid_(true), sum_local_uncached_popularity_(sum_local_uncached_popularity), smallest_max_admission_benefit_(smallest_max_admission_benefit)
    {
        assert(isValid());
    }

    FastPathHint::~FastPathHint() {}

    bool FastPathHint::isValid() const
    {
        return is_valid_;
    }

    const Popularity FastPathHint::getSumLocalUncachedPopularity() const
    {
        assert(isValid());
        return sum_local_uncached_popularity_;
    }

    const DeltaReward FastPathHint::getSmallestMaxAdmissionBenefit() const
    {
        assert(isValid());
        return smallest_max_admission_benefit_;
    }

    uint32_t FastPathHint::getFastPathHintPayloadSize() const
    {
        assert(isValid());
        return sizeof(Popularity) + sizeof(DeltaReward);
    }

    uint32_t FastPathHint::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&sum_local_uncached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        msg_payload.deserialize(size, (const char*)&smallest_max_admission_benefit_, sizeof(DeltaReward));
        size += sizeof(DeltaReward);
        return size - position;
    }

    uint32_t FastPathHint::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char *)&sum_local_uncached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        msg_payload.serialize(size, (char *)&smallest_max_admission_benefit_, sizeof(DeltaReward));
        size += sizeof(DeltaReward);
        is_valid_ = true;
        return size - position;
    }

    const FastPathHint& FastPathHint::operator=(const FastPathHint& other)
    {
        is_valid_ = other.is_valid_;
        sum_local_uncached_popularity_ = other.sum_local_uncached_popularity_;
        smallest_max_admission_benefit_ = other.smallest_max_admission_benefit_;
        return *this;
    }
}