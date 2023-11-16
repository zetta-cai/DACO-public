#include "cache/covered/reward_compare.h"

namespace covered
{
    bool RewardLruCompare::operator()(const Reward& lhs, const Reward& rhs) const
    {
        return lhs < rhs; // Ascending order
    }

    bool RewardMruCompare::operator()(const Reward& lhs, const Reward& rhs) const
    {
        return lhs > rhs; // Descending order
    }
}