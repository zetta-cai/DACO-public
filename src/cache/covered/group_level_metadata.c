#include "cache/covered/group_level_metadata.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string GroupLevelMetadata::kClassName("GroupLevelMetadata");

    GroupLevelMetadata::GroupLevelMetadata()
    {
        #ifndef TRACK_PERKEY_OBJSIZE
        avg_object_size_ = 0.0;
        #endif
        object_cnt_ = 0;
    }

    GroupLevelMetadata::GroupLevelMetadata(const GroupLevelMetadata& other)
    {
        #ifndef TRACK_PERKEY_OBJSIZE
        avg_object_size_ = other.avg_object_size_;
        #endif
        object_cnt_ = other.object_cnt_;
    }

    GroupLevelMetadata::~GroupLevelMetadata() {}

    void GroupLevelMetadata::updateForNewlyGrouped(const Key& key, const Value& value)
    {
        #ifndef TRACK_PERKEY_OBJSIZE
        uint32_t object_size = key.getKeyLength() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size) / (object_cnt_ + 1);
        #endif
        object_cnt_++;

        return;
    }

    void GroupLevelMetadata::updateNoValueStatsForInGroupKey()
    {
        // NO value-unrelated metadata to update for existing keys
        
        return;
    }

    void GroupLevelMetadata::updateValueStatsForInGroupKey(const Key& key, const Value& value, const Value& original_value)
    {
        assert(object_cnt_ > 0);

        #ifndef TRACK_PERKEY_OBJSIZE
        uint32_t original_object_size = key.getKeyLength() + original_value.getValuesize();
        uint32_t object_size = key.getKeyLength() + value.getValuesize();
        if (avg_object_size_ * object_cnt_ + object_size >= static_cast<AvgObjectSize>(original_object_size))
        {
            avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size - original_object_size) / object_cnt_;
        }
        else
        {
            std::ostringstream oss;
            oss << "avg_object_size_ * object_cnt_ (" << avg_object_size_ << " * " << object_cnt_ << ") + object_size (" << object_size << ") < original_object_size (" << original_object_size << ")";
            Util::dumpWarnMsg(kClassName, oss.str());

            avg_object_size_ = 0.0;
        }
        #endif

        return;
    }

    bool GroupLevelMetadata::updateForDegrouped(const Key& key, const Value& value, const bool& need_warning)
    {
        #ifndef TRACK_PERKEY_OBJSIZE
        uint32_t object_size = key.getKeyLength() + value.getValuesize();
        if (object_cnt_ > 1)
        {
            if (avg_object_size_ * object_cnt_ >= static_cast<AvgObjectSize>(object_size))
            {
                avg_object_size_ = (avg_object_size_ * object_cnt_ - object_size) / (object_cnt_ - 1);
            }
            else
            {
                if (need_warning)
                {
                    std::ostringstream oss;
                    oss << "Key " << key.getKeystr() << ": avg_object_size_ * object_cnt_ (" << avg_object_size_ << " * " << object_cnt_ << ") < object_size (" << object_size << ")";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }

                avg_object_size_ = 0.0;
            }
        }
        else
        {
            avg_object_size_ = 0.0;
        }
        #endif

        object_cnt_--;

        bool is_group_empty = (object_cnt_ == 0);
        return is_group_empty;
    }

    #ifndef TRACK_PERKEY_OBJSIZE
    AvgObjectSize GroupLevelMetadata::getAvgObjectSize() const
    {
        assert(object_cnt_ > 0);
        return avg_object_size_;
    }
    #endif
    
    ObjectCnt GroupLevelMetadata::getObjectCnt() const
    {
        return object_cnt_;
    }

    uint64_t GroupLevelMetadata::getSizeForCapacity()
    {
        #ifndef TRACK_PERKEY_OBJSIZE
        return sizeof(AvgObjectSize) + sizeof(ObjectCnt);
        #else
        return sizeof(ObjectCnt);
        #endif
    }
}