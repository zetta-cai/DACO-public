/*
 * RewardLruCompare and RewardMruCompare: compare class to sort reward for reward-based popularity tracking.
 *
 * By Siyuan Sheng (2023.11.16).
 */

#include "common/covered_common_header.h"

namespace covered
{
    // LRU for objects with the same reward (e.g., zero-reward one-hit-wonders)
    class RewardLruCompare
    {
    public:
        bool operator()(const Reward& lhs, const Reward& rhs) const;
    };

    // MRU for objects with the same reward (e.g., zero-reward one-hit-wonders)
    class RewardMruCompare
    {
    public:
        bool operator()(const Reward& lhs, const Reward& rhs) const;
    };
}