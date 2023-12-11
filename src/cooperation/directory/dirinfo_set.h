/*
 * DirinfoSet: a set of DirectoryInfo (support compressed mode for dedup-/delta-compression-based victim synchronization).
 * 
 * By Siyuan Sheng (2023.10.17).
 */

#ifndef DIRINFO_SET_H
#define DIRINFO_SET_H

//#define DEBUG_DIRINFO_SET

#include <string>
#include <unordered_set>

#include "common/key.h"
#include "cooperation/directory/directory_info.h"

namespace covered
{
    class DirinfoSet
    {
    public:
        static DirinfoSet compress(const DirinfoSet& current_dirinfo_set, const DirinfoSet& prev_dirinfo_set);
        static DirinfoSet recover(const DirinfoSet& compressed_dirinfo_set, const DirinfoSet& existing_dirinfo_set);

        static std::list<std::pair<Key, DirinfoSet>>::iterator findDirinfoSetForKey(const Key& key, std::list<std::pair<Key, DirinfoSet>>& dirinfo_sets); // Find DirinfoSet for the given key
        static std::list<std::pair<Key, DirinfoSet>>::const_iterator findDirinfoSetForKey(const Key& key, const std::list<std::pair<Key, DirinfoSet>>& dirinfo_sets); // Find DirinfoSet for the given key

        DirinfoSet();
        DirinfoSet(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set);
        ~DirinfoSet();

        bool isInvalid() const;
        bool isComplete() const;
        bool isCompressed() const; // Delta compression
        bool isFullyCompressed() const; // Whether both new and stale dirinfo delta sets are empty

        // For complete dirinfo set
        bool isExistIfComplete(const DirectoryInfo& directory_info, bool& is_exist) const; // Return if with complete dirinfo set
        bool getDirinfoSetSizeIfComplete(uint32_t& dirinfo_set_size) const; // Return if with complete dirinfo set
        bool tryToInsertIfComplete(const DirectoryInfo& directory_info, bool& is_insert); // Return if with complete dirinfo set
        bool tryToEraseIfComplete(const DirectoryInfo& directory_info, bool& is_erase); // Return if with complete dirinfo set
        bool getDirinfoIfComplete(const uint32_t advance_idx, DirectoryInfo& directory_info) const; // Return if with complete dirinfo set
        bool getDirinfoSetIfComplete(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const; // Return if with complete dirinfo set
        void setDirinfoSetForComplete(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set);

        // For compressed dirinfo set
        bool getDeltaDirinfoSetIfCompressed(std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& new_dirinfo_delta_set, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& stale_dirinfo_delta_set) const; // Return if with complete dirinfo set
        void setDeltaDirinfoSetForCompress(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& new_dirinfo_delta_set, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& stale_dirinfo_delta_set);

        uint32_t getDirinfoSetPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const DirinfoSet& operator=(const DirinfoSet& other);
    private:
        static const std::string kClassName;

        static const uint8_t INVALID_BITMAP; // Invalid bitmap
        static const uint8_t COMPLETE_BITMAP; // All fields are NOT delta-compressed
        static const uint8_t DELTA_MASK; // At least one filed is delta-compressed
        static const uint8_t NEW_DIRINFO_SET_DELTA_MASK; // Whether new dirinfo set exists after delta compression
        static const uint8_t STALE_DIRINFO_SET_DELTA_MASK; // Whether stale dirinfo set exists after delta compression

        uint32_t getDirinfoSetPayloadSizeInternal_(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const;
        uint32_t serializeDirinfoSetInternal_(DynamicArray& msg_payload, const uint32_t& position, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const;
        uint32_t deserializeDirinfoSetInternal_(const DynamicArray& msg_payload, const uint32_t& position, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set);

        uint32_t getDirinfoSetSizeForCapacityInternal_(const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& dirinfo_set) const;

        uint8_t delta_bitmap_; // 1st lowest bit indicates if the dirinfo set is a compressed dirinfo set (2nd lowest bit for new dirinfo delta set; 3rd lowest bit for stale dirinfo delta set)

        // For complete dirinfo set
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> dirinfo_set_;
        // For compressed dirinfo set
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> new_dirinfo_delta_set_;
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> stale_dirinfo_delta_set_;
    };
}

#endif