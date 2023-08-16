#include "cache/covered/pergroup_statistics.h"

namespace covered
{
    const std::string PergroupStatistics::kClassName("PergroupStatistics");

    PergroupStatistics::PergroupStatistics()
    {
        avg_object_size_ = 0;
        object_cnt_ = 0;
    }

    PergroupStatistics::~PergroupStatistics() {}

    void PergroupStatistics::updateForNewlyGroupedKey(const Key& key, const Value& value)
    {
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size) / (object_cnt_ + 1);
        object_cnt_++;
        return;
    }

    void PergroupStatistics::updateForInGroupKey(const Key& key)
    {
        // NO need to update avg_object_size_ due to unchanged value size
        
        return;
    }

    void PergroupStatistics::updateForInGroupKey(const Key& key, const Value& value, const Value& original_value)
    {
        uint32_t original_object_size = key.getKeystr().length() + original_value.getValuesize();
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ - original_object_size + object_size) / object_cnt_;
        return;
    }

    void PergroupStatistics::updateForDegroupedKey(const Key& key, const Value& original_value)
    {
        uint32_t original_object_size = key.getKeystr().length() + original_value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ - original_object_size) / (object_cnt_ - 1);
        object_cnt_--;
        return;
    }

    uint32_t PergroupStatistics::getObjectSize() const
    {
        return avg_object_size_;
    }
    
    uint32_t PergroupStatistics::getObjectCnt() const
    {
        return object_cnt_;
    }
}