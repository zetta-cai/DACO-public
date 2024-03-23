#include "cache/covered/group_level_metadata.h"

#include <assert.h>
#include <sstream>

namespace covered
{
    const std::string GroupLevelMetadata::kClassName("GroupLevelMetadata");

    GroupLevelMetadata::GroupLevelMetadata()
    {
        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        avg_object_size_ = 0.0;
        #endif
        object_cnt_ = 0;
    }

    GroupLevelMetadata::GroupLevelMetadata(const GroupLevelMetadata& other)
    {
        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        avg_object_size_ = other.avg_object_size_;
        #endif
        object_cnt_ = other.object_cnt_;
    }

    GroupLevelMetadata::~GroupLevelMetadata() {}

    void GroupLevelMetadata::updateForNewlyGrouped(const Key& key, const Value& value)
    {
        // NO value-unrelated metadata to update for newly-grouped keys

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        // Update value-related metadata for newly-grouped keys
        uint32_t object_size = key.getKeyLength() + value.getValuesize();
        avg_object_size_ = (avg_object_size_ * object_cnt_ + object_size) / (object_cnt_ + 1);
        #endif

        // Update object count
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

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
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

    bool GroupLevelMetadata::updateForDegrouped(const Key& key, const Value& value)
    {
        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        uint32_t object_size = key.getKeyLength() + value.getValuesize();
        if (object_cnt_ > 1)
        {
            if (avg_object_size_ * object_cnt_ >= static_cast<AvgObjectSize>(object_size))
            {
                avg_object_size_ = (avg_object_size_ * object_cnt_ - object_size) / (object_cnt_ - 1);
            }
            else
            {
                // NOTE: this is possible under approximate value size for local uncached metadata
                // std::ostringstream oss;
                // oss << "Key " << key.getKeyDebugstr() << ": avg_object_size_ * object_cnt_ (" << avg_object_size_ << " * " << object_cnt_ << ") < object_size (" << object_size << ")";
                // Util::dumpWarnMsg(kClassName, oss.str());

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

    #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
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
        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        return sizeof(AvgObjectSize) + sizeof(ObjectCnt);
        #else
        return sizeof(ObjectCnt);
        #endif
    }

    // Dump/load group-level metadata for local cached/uncached metadata of cache metadata in cache snapshot

    void GroupLevelMetadata::dumpGroupLevelMetadata(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        // Dump avg object size
        fs_ptr->write((const char*)&avg_object_size_, sizeof(AvgObjectSize));
        #endif

        // Dump object cnt
        fs_ptr->write((const char*)&object_cnt_, sizeof(ObjectCnt));

        return;
    }

    void GroupLevelMetadata::loadGroupLevelMetadata(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        #ifndef ENABLE_TRACK_PERKEY_OBJSIZE
        // Load avg object size
        fs_ptr->read((char*)&avg_object_size_, sizeof(AvgObjectSize));
        #endif

        // Load object cnt
        fs_ptr->read((char*)&object_cnt_, sizeof(ObjectCnt));

        return;
    }
}