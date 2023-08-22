#include "cache/covered/group_level_statistics.h"

namespace covered
{
    const std::string GroupLevelStatistics::kClassName("GroupLevelStatistics");

    GroupLevelStatistics::GroupLevelStatistics()
    {
        avg_object_size_ = 0;
        object_cnt_ = 0;
    }

    GroupLevelStatistics::~GroupLevelStatistics() {}

    void GroupLevelStatistics::updateForNewlyGrouped(const Key& key, const Value& value)
    {
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size) / (object_cnt_ + 1);
        object_cnt_++;
        return;
    }

    void GroupLevelStatistics::updateForInGroupKey(const Key& key)
    {
        // NO need to update avg_object_size_ due to unchanged value size
        
        return;
    }

    void GroupLevelStatistics::updateForInGroupKeyValue(const Key& key, const Value& value, const Value& original_value)
    {
        uint32_t original_object_size = key.getKeystr().length() + original_value.getValuesize();
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ - original_object_size + object_size) / object_cnt_;
        return;
    }

    void GroupLevelStatistics::updateForDegrouped(const Key& key, const Value& original_value)
    {
        uint32_t original_object_size = key.getKeystr().length() + original_value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ - original_object_size) / (object_cnt_ - 1);
        object_cnt_--;
        return;
    }

    uint32_t GroupLevelStatistics::getAvgObjectSize() const
    {
        return avg_object_size_;
    }
    
    uint32_t GroupLevelStatistics::getObjectCnt() const
    {
        return object_cnt_;
    }
}