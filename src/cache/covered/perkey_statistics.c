#include "cache/covered/perkey_statistics.h"

namespace covered
{
    const std::string PerkeyStatistics::kClassName("PerkeyStatistics");

    PerkeyStatistics::PerkeyStatistics()
    {
        group_id_ = 0;
        frequency_ = 0;
    }

    PerkeyStatistics::~PerkeyStatistics() {}

    void PerkeyStatistics::update()
    {
        frequency_++;
        return;
    }

    uint32_t PerkeyStatistics::getGroupId() const
    {
        return group_id_;
    }

    uint32_t PerkeyStatistics::getFrequency() const
    {
        return frequency_;
    }
}