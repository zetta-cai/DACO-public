#include "cache/covered/perkey_statistics_list.h"

namespace covered
{
    const std::string PerkeyStatisticsList::kClassName("PerkeyStatisticsList");

    PerkeyStatisticsList::PerkeyStatisticsList()
    {
        perkey_statistics_list_.clear();
    }
    
    PerkeyStatisticsList::~PerkeyStatisticsList() {}
}