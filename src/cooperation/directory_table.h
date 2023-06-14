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

        // TODO: use std::atomic<bool> if edge node has multiple sub-threads (now each edge node only has 1 thread to process data/control requests)
        bool is_valid_;
    };

    // A directory entry stores multiple directory infos and metadatas for a given key
    class DirectoryEntry
    {
    public:
        DirectoryEntry();
        ~DirectoryEntry();

        // NOTE: isBeingWritten() and getValidDirinfoSet() cannot be const due to try_lock_shared()
        // NOTE: the following methods return whether the key is being written
        bool isBeingWritten();
        bool getValidDirinfoSet(dirinfo_set_t& dirinfo_set);
        bool addDirinfo(const DirectoryInfo& directory_info); // TODO: END HERE
    private:
        typedef std::unordered_map<DirectoryInfo, DirectoryMetadata, DirectoryInfoHasher> dirinfo_entry_t;

        static const std::string kClassName;

        dirinfo_entry_t directory_entry_;
        boost::shared_mutex rwlock_;
    };

    // A table maps key into corresponding directory entry
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed);
        ~DirectoryTable();

        // NOTE: lookup() cannot be const due to getValidDirinfoSet()
        void lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info);
        void update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);
    private:
        typedef std::unordered_map<Key, DirectoryEntry, KeyHasher> dirinfo_table_t;

        static const std::string kClassName;

        dirinfo_table_t directory_hashtable_; // Maintain directory information (not need ordered map)
        std::mt19937_64* directory_randgen_ptr_;
    };
}

#endif