/*
 * DirectoryTable: manage directory information of different keys.
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef DIRECTORY_TABLE_H
#define DIRECTORY_TABLE_H

#include <random> // std::mt19937_64
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"
#include "cooperation/directory_info.h"

namespace covered
{
    typedef std::unordered_set<DirectoryInfo, DirectoryInfoHasher> dirinfo_set_t;

    class DirectoryMetadata
    {
    public:
        DirectoryMetadata(const bool& is_valid);
        ~DirectoryMetadata();

        bool isValid() const;
        void validate();
        void invalidate();
    private:
        static const std::string kClassName;

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        bool is_valid_;
    };

    // A directory entry stores multiple directory infos and metadatas for the given key
    class DirectoryEntry
    {
    public:
        DirectoryEntry();
        ~DirectoryEntry();

        bool isBeingWritten() const;
        void getValidDirinfoSet(dirinfo_set_t& dirinfo_set) const;
        bool addDirinfo(const DirectoryInfo& directory_info); // return is_directory_already_exist
        bool removeDirinfo(const DirectoryInfo& directory_info); // return is_directory_already_exist
    private:
        typedef std::unordered_map<DirectoryInfo, DirectoryMetadata, DirectoryInfoHasher> dirinfo_entry_t;

        static const std::string kClassName;
        static const uint32_t DIRINFO_BUFFER_CAPACITY;
        
        bool addDirinfoInternal_(const DirectoryInfo& directory_info, const bool& is_valid); // return is_directory_already_exist

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        dirinfo_entry_t directory_entry_;
        bool is_being_written_; // Up to one write is allowed for the given key each time
    };

    // A table maps key into corresponding directory entry
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed);
        ~DirectoryTable();

        // NOTE: lookup() cannot be const due to rwlock_.try_lock_shared()
        void lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info);
        void update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);
    private:
        typedef std::unordered_map<Key, DirectoryEntry, KeyHasher> dirinfo_table_t;

        static const std::string kClassName;

        dirinfo_table_t directory_hashtable_; // Maintain directory information (not need ordered map)
        std::mt19937_64* directory_randgen_ptr_;

        // Reader: lookup() for control thread(s); writer: update() for control thread(s), invalidateEntry() and finishWrite() for data thread(s) with writes
        boost::shared_mutex rwlock_; // rwlock_ guarantees that up to one writer is allowed each time
    };
}

#endif