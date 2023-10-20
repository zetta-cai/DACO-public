#include "cooperation/directory/dirinfo_set.h"

#include "common/util.h"

namespace covered
{
    const std::string DirinfoSet::kClassName = "DirinfoSet";

    const uint8_t DirinfoSet::INVALID_BITMAP = 0b11111111;
    const uint8_t DirinfoSet::COMPLETE_BITMAP = 0b00000000;
    const uint8_t DirinfoSet::DELTA_MASK = 0b00000001;
    const uint8_t DirinfoSet::NEW_DIRINFO_SET_DELTA_MASK = 0b00000010 | DELTA_MASK;
    const uint8_t DirinfoSet::STALE_DIRINFO_SET_DELTA_MASK = 0b00000100 | DELTA_MASK;

    DirinfoSet DirinfoSet::compress(const DirinfoSet& current_dirinfo_set, const DirinfoSet& prev_dirinfo_set)
    {
        // Perform delta compression based on two complete dirinfo sets
        assert(current_dirinfo_set.isComplete());
        assert(prev_dirinfo_set.isComplete());

        DirinfoSet compressed_dirinfo_set = current_dirinfo_set;

        // (1) Perform delta compression on the set of dirinfos

        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> new_dirinfo_set;
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> stale_dirinfo_set;
        bool with_complete_dirinfo_set = true;
        uint32_t total_payload_size_for_current_dirinfo_set = 0;
        uint32_t total_payload_size_for_final_dirinfo_set = 0;

        // Calculate delta dirinfos
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> current_dirinfo_unordered_set;
        bool current_with_complete_dirinfo_set = current_dirinfo_set.getDirinfoSetIfComplete(current_dirinfo_unordered_set);
        assert(current_with_complete_dirinfo_set == true);
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> prev_dirinfo_unordered_set;
        bool prev_with_complete_dirinfo_set = prev_dirinfo_set.getDirinfoSetIfComplete(prev_dirinfo_unordered_set);
        assert(prev_with_complete_dirinfo_set == true);
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator current_dirinfo_unordered_set_const_iter = current_dirinfo_unordered_set.begin(); current_dirinfo_unordered_set_const_iter != current_dirinfo_unordered_set.end(); current_dirinfo_unordered_set_const_iter++) // Get new dirinfos
        {
            const DirectoryInfo& tmp_current_dirinfo = *current_dirinfo_unordered_set_const_iter;
            total_payload_size_for_current_dirinfo_set += tmp_current_dirinfo.getDirectoryInfoPayloadSize();

            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator tmp_prev_dirinfo_unordered_set_const_iter = prev_dirinfo_unordered_set.find(tmp_current_dirinfo);
            if (tmp_prev_dirinfo_unordered_set_const_iter == prev_dirinfo_unordered_set.end()) // New dirinfo (with current yet without prev)
            {
                new_dirinfo_set.insert(tmp_current_dirinfo);
                total_payload_size_for_final_dirinfo_set += tmp_current_dirinfo.getDirectoryInfoPayloadSize();
            }
        }
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator prev_dirinfo_unordered_set_const_iter = prev_dirinfo_unordered_set.begin(); prev_dirinfo_unordered_set_const_iter != prev_dirinfo_unordered_set.end(); prev_dirinfo_unordered_set_const_iter++) // Get stale dirinfos
        {
            const DirectoryInfo& tmp_prev_dirinfo = *prev_dirinfo_unordered_set_const_iter;
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator tmp_current_dirinfo_unordered_set_const_iter = current_dirinfo_unordered_set.find(tmp_prev_dirinfo);
            if (tmp_current_dirinfo_unordered_set_const_iter == current_dirinfo_unordered_set.end()) // Stale dirinfo (with prev yet without current)
            {
                stale_dirinfo_set.insert(tmp_prev_dirinfo);
                total_payload_size_for_final_dirinfo_set += tmp_prev_dirinfo.getDirectoryInfoPayloadSize();
            }
        }

        // Set delta dirinfos for compression if necessary
        with_complete_dirinfo_set = (total_payload_size_for_current_dirinfo_set <= total_payload_size_for_final_dirinfo_set);
        if (!with_complete_dirinfo_set)
        {
            compressed_dirinfo_set.setDeltaDirinfoSetForCompress(new_dirinfo_set, stale_dirinfo_set);
        }

        // (2) Get final dirinfo set
        const uint32_t compressed_dirinfo_set_payload_size = compressed_dirinfo_set.getDirinfoSetPayloadSize();
        const uint32_t current_dirinfo_set_payload_size = current_dirinfo_set.getDirinfoSetPayloadSize();
        if (compressed_dirinfo_set_payload_size < current_dirinfo_set_payload_size)
        {
            #ifdef DEBUG_DIRINFO_SET
            Util::dumpVariablesForDebug(kClassName, 5, "use compressed victim dirinfo set!", "compressed_dirinfo_set_payload_size:", std::to_string(compressed_dirinfo_set_payload_size).c_str(), "current_dirinfo_set_payload_size:", std::to_string(current_dirinfo_set_payload_size).c_str());
            #endif

            return compressed_dirinfo_set;
        }
        else
        {
            #ifdef DEBUG_DIRINFO_SET
            Util::dumpVariablesForDebug(kClassName, 5, "use complete victim dirinfo set!", "compressed_dirinfo_set_payload_size:", std::to_string(compressed_dirinfo_set_payload_size).c_str(), "current_dirinfo_set_payload_size:", std::to_string(current_dirinfo_set_payload_size).c_str());
            #endif

            return current_dirinfo_set;
        }
    }

    DirinfoSet DirinfoSet::recover(const DirinfoSet& compressed_dirinfo_set, const DirinfoSet& existing_dirinfo_set)
    {
        // Recover complete dirinfo set based on compressed one and existing complete one
        assert(compressed_dirinfo_set.isCompressed());
        assert(existing_dirinfo_set.isComplete());

        DirinfoSet complete_dirinfo_set = existing_dirinfo_set;

        // (1) Recover complete dirinfo unordered set

        // Start from existing complete dirinfo unordered set
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> complete_dirinfo_unordered_set;
        bool tmp_with_complete_dirinfo_unordered_set = existing_dirinfo_set.getDirinfoSetIfComplete(complete_dirinfo_unordered_set);
        assert(tmp_with_complete_dirinfo_unordered_set);

        // Get delta dirinfo sets
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> new_dirinfo_set;
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> stale_dirinfo_set;
        tmp_with_complete_dirinfo_unordered_set = compressed_dirinfo_set.getDeltaDirinfoSetIfCompressed(new_dirinfo_set, stale_dirinfo_set);
        assert(!tmp_with_complete_dirinfo_unordered_set);

        // Add new dirinfos into complete dirinfo set
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator new_dirinfo_set_const_iter = new_dirinfo_set.begin(); new_dirinfo_set_const_iter != new_dirinfo_set.end(); new_dirinfo_set_const_iter++)
        {
            const DirectoryInfo& tmp_dirinfo = *new_dirinfo_set_const_iter;
            if (complete_dirinfo_unordered_set.find(tmp_dirinfo) == complete_dirinfo_unordered_set.end())
            {
                complete_dirinfo_unordered_set.insert(tmp_dirinfo);
            }
        }

        // Remove stale dirinfos from complete dirinfo set
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator stale_dirinfo_set_const_iter = stale_dirinfo_set.begin(); stale_dirinfo_set_const_iter != stale_dirinfo_set.end(); stale_dirinfo_set_const_iter++)
        {
            const DirectoryInfo& tmp_dirinfo = *stale_dirinfo_set_const_iter;
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator tmp_complete_dirinfo_unordered_set_const_iter = complete_dirinfo_unordered_set.find(tmp_dirinfo);
            if (tmp_complete_dirinfo_unordered_set_const_iter != complete_dirinfo_unordered_set.end())
            {
                complete_dirinfo_unordered_set.erase(tmp_complete_dirinfo_unordered_set_const_iter);
            }
        }

        // Replace existing complete dirinfo set with recovered one
        complete_dirinfo_set.setDirinfoSetForComplete(complete_dirinfo_unordered_set);

        assert(complete_dirinfo_set.isComplete());
        return complete_dirinfo_set;
    }

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

    bool DirinfoSet::isInvalid() const
    {
        return (delta_bitmap_ == INVALID_BITMAP);
    }

    bool DirinfoSet::isComplete() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        return (delta_bitmap_ & DELTA_MASK == COMPLETE_BITMAP & DELTA_MASK);
    }

    bool DirinfoSet::isCompressed() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        return (delta_bitmap_ & DELTA_MASK) == DELTA_MASK;
    }

    bool DirinfoSet::isFullyCompressed() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        bool is_fully_compressed = ((delta_bitmap_ & DELTA_MASK) == DELTA_MASK) && ((delta_bitmap_ & NEW_DIRINFO_SET_DELTA_MASK) != NEW_DIRINFO_SET_DELTA_MASK) && ((delta_bitmap_ & STALE_DIRINFO_SET_DELTA_MASK) != STALE_DIRINFO_SET_DELTA_MASK);

        if (is_fully_compressed)
        {
            assert(new_dirinfo_delta_set_.size() == 0);
            assert(stale_dirinfo_delta_set_.size() == 0);
        }

        return is_fully_compressed;
    }

    // For complete dirinfo set

    bool DirinfoSet::isExistIfComplete(const DirectoryInfo& directory_info, bool& is_exist) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        bool with_complete_dirinfo_set = isComplete();
        if (with_complete_dirinfo_set)
        {
            // Check if directory info exists
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_set_const_iter = dirinfo_set_.find(directory_info);
            is_exist = (dirinfo_set_const_iter != dirinfo_set_.end());
        }
        
        return with_complete_dirinfo_set;
    }

    bool DirinfoSet::getDirinfoSetSizeIfComplete(uint32_t& dirinfo_set_size) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        bool with_complete_dirinfo_set = isComplete();
        if (with_complete_dirinfo_set)
        {
            dirinfo_set_size = dirinfo_set_.size();
        }

        return with_complete_dirinfo_set;
    }

    bool DirinfoSet::tryToInsertIfComplete(const DirectoryInfo& directory_info, bool& is_insert)
    {
        bool with_complete_dirinfo_set = isComplete();
        is_insert = false;
        if (with_complete_dirinfo_set)
        {
            // Try to insert directory info if not exists
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_set_const_iter = dirinfo_set_.find(directory_info);
            if (dirinfo_set_const_iter == dirinfo_set_.end())
            {
                dirinfo_set_.insert(directory_info);
                is_insert = true;
            }
        }
        return with_complete_dirinfo_set;
    }

    bool DirinfoSet::tryToEraseIfComplete(const DirectoryInfo& directory_info, bool& is_erase)
    {
        bool with_complete_dirinfo_set = isComplete();
        is_erase = false;
        if (with_complete_dirinfo_set)
        {
            // Try to erase directory info if exists
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_set_const_iter = dirinfo_set_.find(directory_info);
            if (dirinfo_set_const_iter != dirinfo_set_.end())
            {
                dirinfo_set_.erase(dirinfo_set_const_iter);
                is_erase = true;
            }
        }
        return with_complete_dirinfo_set;
    }

    bool DirinfoSet::getDirinfoIfComplete(const uint32_t advance_idx, DirectoryInfo& directory_info) const
    {
        bool with_complete_dirinfo_set = isComplete();
        if (with_complete_dirinfo_set)
        {
            assert(advance_idx < dirinfo_set_.size());

            // Get ith directory info if exists
            std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator dirinfo_set_const_iter = dirinfo_set_.begin();
            std::advance(dirinfo_set_const_iter, advance_idx);
            assert(dirinfo_set_const_iter != dirinfo_set_.end());
            directory_info = *dirinfo_set_const_iter;
        }
        return with_complete_dirinfo_set;
    }

    bool DirinfoSet::getDirinfoSetIfComplete(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        dirinfo_set.clear();

        bool with_complete_dirinfo_set = isComplete();
        if (with_complete_dirinfo_set)
        {
            dirinfo_set = dirinfo_set_;
        }

        return with_complete_dirinfo_set;
    }

    void DirinfoSet::setDirinfoSetForComplete(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set)
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        delta_bitmap_ = COMPLETE_BITMAP;
        new_dirinfo_delta_set_.clear();
        stale_dirinfo_delta_set_.clear();
        dirinfo_set_ = dirinfo_set;

        return;
    }

    // For compressed dirinfo set

    bool DirinfoSet::getDeltaDirinfoSetIfCompressed(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& new_dirinfo_delta_set, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& stale_dirinfo_delta_set) const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        new_dirinfo_delta_set.clear();
        stale_dirinfo_delta_set.clear();

        bool with_complete_dirinfo_set = isComplete();
        if (!with_complete_dirinfo_set)
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

    void DirinfoSet::setDeltaDirinfoSetForCompress(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& new_dirinfo_delta_set, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& stale_dirinfo_delta_set)
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        delta_bitmap_ |= DELTA_MASK;
        dirinfo_set_.clear();

        if (new_dirinfo_delta_set.size() > 0)
        {
            delta_bitmap_ |= NEW_DIRINFO_SET_DELTA_MASK;
            new_dirinfo_delta_set_ = new_dirinfo_delta_set;
        }

        if (stale_dirinfo_delta_set.size() > 0)
        {
            delta_bitmap_ |= STALE_DIRINFO_SET_DELTA_MASK;
            stale_dirinfo_delta_set_ = stale_dirinfo_delta_set;
        }

        return;
    }

    uint32_t DirinfoSet::getDirinfoSetPayloadSize() const
    {
        assert(delta_bitmap_ != INVALID_BITMAP);

        uint32_t dirinfo_set_payload_size = 0;

        dirinfo_set_payload_size += sizeof(uint8_t); // delta bitmap
        bool with_complete_dirinfo_set = isComplete();
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
        bool with_complete_dirinfo_set = isComplete();
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
        bool with_complete_dirinfo_set = isComplete();
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
        bool with_complete_dirinfo_set = isComplete();
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