#include "cooperation/directory/dirinfo_set.h"

namespace covered
{
    const std::string DirinfoSet::kClassName = "DirinfoSet";

    const uint8_t DirinfoSet::INVALID_BITMAP = 0b11111111;
    const uint8_t DirinfoSet::COMPLETE_BITMAP = 0b00000000;
    const uint8_t DirinfoSet::NEW_DIRINFO_SET_DELTA_MASK = 0b00000011;
    const uint8_t DirinfoSet::STALE_DIRINFO_SET_DELTA_MASK = 0b00000101;

    DirinfoSet::DirinfoSet() : dirinfo_set_(), new_dirinfo_delta_set_(), stale_dirinfo_delta_set_()
    {
        delta_bitmap_ = INVALID_BITMAP;
    }

    DirinfoSet::DirinfoSet(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) : new_dirinfo_delta_set_(), stale_dirinfo_delta_set_()
    {
        delta_bitmap_ = COMPLETE_BITMAP;
        dirinfo_set_ = dirinfo_set;
    }

    DirinfoSet::~DirinfoSet()
    {
    }

    bool DirinfoSet::isComplete() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        return (delta_bitmap_ == COMPLETE_BITMAP);
    }

    bool DirinfoSet::isCompressed() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        return (delta_bitmap_ != COMPLETE_BITMAP);
    }

    bool DirinfoSet::getDirinfoSetOrDelta(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& new_dirinfo_delta_set, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& stale_dirinfo_delta_set) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        dirinfo_set.clear();
        new_dirinfo_delta_set.clear();
        stale_dirinfo_delta_set.clear();

        bool with_complete_dirinfo_set = (delta_bitmap_ == COMPLETE_BITMAP);

        if (with_complete_dirinfo_set)
        {
            dirinfo_set = dirinfo_set_;
        }
        else
        {
            if ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) == NEW_DIRINFO_SET_DELTA_MASK)
            {
                assert(new_dirinfo_delta_set_.size() > 0);

                new_dirinfo_delta_set = new_dirinfo_delta_set_;
            }
            if ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) == STALE_DIRINFO_SET_DELTA_MASK)
            {
                assert(stale_dirinfo_delta_set_.size() > 0);

                stale_dirinfo_delta_set = stale_dirinfo_delta_set_;
            }
        }

        return with_complete_dirinfo_set;
    }

    uint32_t DirinfoSet::getDirinfoSetPayloadSize() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        uint32_t dirinfo_set_payload_size = 0;

        dirinfo_set_payload_size += sizeof(uint8_t); // delta bitmap
        bool with_complete_dirinfo_set = (delta_bitmap_ == COMPLETE_BITMAP);
        if (with_complete_dirinfo_set)
        {
            dirinfo_set_payload_size += getDirinfoSetPayloadSizeInternal_(dirinfo_set_);
        }
        else
        {
            if ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) == NEW_DIRINFO_SET_DELTA_MASK)
            {
                assert(new_dirinfo_delta_set_.size() > 0);

                getDirinfoSetPayloadSizeInternal_(new_dirinfo_delta_set_);
            }
            if ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) == STALE_DIRINFO_SET_DELTA_MASK)
            {
                assert(stale_dirinfo_delta_set_.size() > 0);

                getDirinfoSetPayloadSizeInternal_(stale_dirinfo_delta_set_);
            }
        }

        return dirinfo_set_payload_size;
    }

    uint32_t DirinfoSet::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&delta_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        bool with_complete_dirinfo_set = (delta_bitmap_ == COMPLETE_BITMAP);
        if (with_complete_dirinfo_set)
        {
            uint32_t dirinfo_set_serialize_size = serializeDirinfoSetInternal_(msg_payload, size, dirinfo_set_);
            size += dirinfo_set_serialize_size;
        }
        else
        {
            if ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) == NEW_DIRINFO_SET_DELTA_MASK)
            {
                assert(new_dirinfo_delta_set_.size() > 0);

                uint32_t new_dirinfo_delta_set_serialize_size = serializeDirinfoSetInternal_(msg_payload, size, new_dirinfo_delta_set_);
                size += new_dirinfo_delta_set_serialize_size;
            }
            if ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) == STALE_DIRINFO_SET_DELTA_MASK)
            {
                assert(stale_dirinfo_delta_set_.size() > 0);

                uint32_t stale_dirinfo_delta_set_serialize_size = serializeDirinfoSetInternal_(msg_payload, size, stale_dirinfo_delta_set_);
                size += stale_dirinfo_delta_set_serialize_size;
            }
        }

        return size - position;
    }

    uint32_t DirinfoSet::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        uint32_t size = position;
        msg_payload.serialize(size, (char*)&delta_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        bool with_complete_dirinfo_set = (delta_bitmap_ == COMPLETE_BITMAP);
        if (with_complete_dirinfo_set)
        {
            uint32_t dirinfo_set_deserialize_size = deserializeDirinfoSetInternal_(msg_payload, size, dirinfo_set_);
            size += dirinfo_set_deserialize_size;
        }
        else
        {
            if ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) == NEW_DIRINFO_SET_DELTA_MASK)
            {
                assert(new_dirinfo_delta_set_.size() > 0);

                uint32_t new_dirinfo_delta_set_deserialize_size = deserializeDirinfoSetInternal_(msg_payload, size, new_dirinfo_delta_set_);
                size += new_dirinfo_delta_set_deserialize_size;
            }
            if ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) == STALE_DIRINFO_SET_DELTA_MASK)
            {
                assert(stale_dirinfo_delta_set_.size() > 0);

                uint32_t stale_dirinfo_delta_set_deserialize_size = deserializeDirinfoSetInternal_(msg_payload, size, stale_dirinfo_delta_set_);
                size += stale_dirinfo_delta_set_deserialize_size;
            }
        }

        return size - position;
    }

    uint64_t DirinfoSet::getSizeForCapacity() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        uint64_t size_for_capacity = 0;

        size_for_capacity += sizeof(uint8_t); // delta bitmap
        bool with_complete_dirinfo_set = (delta_bitmap_ == COMPLETE_BITMAP);
        if (with_complete_dirinfo_set)
        {
            size_for_capacity += getDirinfoSetSizeForCapacityInternal_(dirinfo_set_);
        }
        else
        {
            if ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) == NEW_DIRINFO_SET_DELTA_MASK)
            {
                assert(new_dirinfo_delta_set_.size() > 0);

                getDirinfoSetSizeForCapacityInternal_(new_dirinfo_delta_set_);
            }
            if ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) == STALE_DIRINFO_SET_DELTA_MASK)
            {
                assert(stale_dirinfo_delta_set_.size() > 0);

                getDirinfoSetSizeForCapacityInternal_(stale_dirinfo_delta_set_);
            }
        }

        return size_for_capacity;
    }

    const DirinfoSet& DirinfoSet::operator=(const DirinfoSet& other)
    {
        delta_bitmap_ = other.delta_bitmap_;
        dirinfo_set_ = other.dirinfo_set_;
        new_dirinfo_delta_set_ = other.new_dirinfo_delta_set_;
        stale_dirinfo_delta_set_ = other.stale_dirinfo_delta_set_;

        return *this;
    }

    uint32_t DirinfoSet::getDirinfoSetPayloadSizeInternal_(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const
    {
        uint32_t dirinfo_set_payload_size = 0;

        dirinfo_set_payload_size += sizeof(uint32_t); // dirinfo set size
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_const_iter = dirinfo_set_.begin(); dirinfo_const_iter != dirinfo_set_.end(); dirinfo_const_iter++)
        {
            dirinfo_set_payload_size += dirinfo_const_iter->getDirectoryInfoPayloadSize(); // complete dirinfos
        }

        return dirinfo_set_payload_size;
    }

    uint32_t DirinfoSet::serializeDirinfoSetInternal_(DynamicArray& msg_payload, const uint32_t& position, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const
    {
        uint32_t size = position;
        uint32_t dirinfo_set_size = dirinfo_set.size();
        msg_payload.deserialize(size, (const char*)&dirinfo_set_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_const_iter = dirinfo_set_.begin(); dirinfo_const_iter != dirinfo_set_.end(); dirinfo_const_iter++)
        {
            uint32_t dirinfo_serialize_size = dirinfo_const_iter->serialize(msg_payload, size);
            size += dirinfo_serialize_size;
        }
        return size - position;
    }

    uint32_t DirinfoSet::deserializeDirinfoSetInternal_(const DynamicArray& msg_payload, const uint32_t& position, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set)
    {
        uint32_t size = position;
        uint32_t dirinfo_set_size = 0;
        msg_payload.serialize(size, (char*)&dirinfo_set_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        for (uint32_t i = 0; i < dirinfo_set_size; i++)
        {
            DirectoryInfo dirinfo;
            uint32_t dirinfo_deserialize_size = dirinfo.deserialize(msg_payload, size);
            size += dirinfo_deserialize_size;
            dirinfo_set.insert(dirinfo);
        }
        return size - position;
    }

    uint32_t DirinfoSet::getDirinfoSetSizeForCapacityInternal_(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const
    {
        uint32_t dirinfo_set_total_size  = 0;
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_const_iter = dirinfo_set_.begin(); dirinfo_const_iter != dirinfo_set_.end(); dirinfo_const_iter++)
        {
            dirinfo_set_total_size += dirinfo_const_iter->getSizeForCapacity(); // complete dirinfos
        }

        return dirinfo_set_total_size;
    }
}