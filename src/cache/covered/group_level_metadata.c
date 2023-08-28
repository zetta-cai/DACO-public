#include "cache/covered/group_level_metadata.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string GroupLevelMetadata::kClassName("GroupLevelMetadata");

    GroupLevelMetadata::GroupLevelMetadata()
    {
        avg_object_size_ = 0;
        object_cnt_ = 0;
    }

    GroupLevelMetadata::GroupLevelMetadata(const GroupLevelMetadata& other)
    {
        avg_object_size_ = other.avg_object_size_;
        object_cnt_ = other.object_cnt_;
    }

    GroupLevelMetadata::~GroupLevelMetadata() {}

    void GroupLevelMetadata::updateForNewlyGrouped(const Key& key, const Value& value)
    {
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size) / (object_cnt_ + 1);
        object_cnt_++;
        return;
    }

    void GroupLevelMetadata::updateForInGroupKey(const Key& key)
    {
        // NO need to update avg_object_size_ due to unchanged value size
        
        return;
    }

    void GroupLevelMetadata::updateForInGroupKeyValue(const Key& key, const Value& value, const Value& original_value)
    {
        assert(object_cnt_ > 0);

        uint32_t original_object_size = key.getKeystr().length() + original_value.getValuesize();
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        if (avg_object_size_ * object_cnt_ + object_size >= original_object_size)
        {
            avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size - original_object_size) / object_cnt_;
        }
        else
        {
            std::ostringstream oss;
            oss << "avg_object_size_ * object_cnt_ (" << avg_object_size_ << " * " << object_cnt_ << ") + object_size (" << object_size << ") < original_object_size (" << original_object_size << ")";
            Util::dumpWarnMsg(kClassName, oss.str());

            avg_object_size_ = 0;
        }
        return;
    }

    bool GroupLevelMetadata::updateForDegrouped(const Key& key, const Value& value)
    {
        uint32_t object_size = key.getKeystr().length() + value.getValuesize();
        if (object_cnt_ > 1)
        {
            if (avg_object_size_ * object_cnt_ >= object_size)
            {
                avg_object_size_ = (avg_object_size_ * object_cnt_ - object_size) / (object_cnt_ - 1);
            }
            else
            {
                std::ostringstream oss;
                oss << "avg_object_size_ * object_cnt_ (" << avg_object_size_ << " * " << object_cnt_ << ") < object_size (" << object_size << ")";
                Util::dumpWarnMsg(kClassName, oss.str());

                avg_object_size_ = 0;
            }
        }
        else
        {
            avg_object_size_ = 0;
        }
        object_cnt_--;

        bool is_group_empty = (object_cnt_ == 0);
        return is_group_empty;
    }

    ObjectSize GroupLevelMetadata::getAvgObjectSize() const
    {
        return avg_object_size_;
    }
    
    ObjectCnt GroupLevelMetadata::getObjectCnt() const
    {
        return object_cnt_;
    }

    uint64_t GroupLevelMetadata::getSizeForCapacity()
    {
        return sizeof(ObjectSize) + sizeof(ObjectCnt);
    }
}