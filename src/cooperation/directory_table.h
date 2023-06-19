/*
 * DirectoryTable: manage directory information of different keys (thread safe).
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef DIRECTORY_TABLE_H
#define DIRECTORY_TABLE_H

#include <random> // std::mt19937_64
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "common/key.h"
#include "cooperation/directory_info.h"
#include "lock/rwlock.h"

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

        uint32_t getSizeForCapacity() const;

        DirectoryMetadata& operator=(const DirectoryMetadata& other);
    private:
        static const std::string kClassName;

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        bool is_valid_; // Metadata for cooperation
    };

    // A directory entry stores multiple directory infos and metadatas for the given key
    class DirectoryEntry
    {
    public:
        DirectoryEntry();
        ~DirectoryEntry();

        void getValidDirinfoSet(dirinfo_set_t& dirinfo_set) const;
        bool addDirinfo(const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata); // return is_directory_already_exist
        bool removeDirinfo(const DirectoryInfo& directory_info); // return is_directory_already_exist

        uint32_t getSizeForCapacity() const;
    private:
        typedef std::unordered_map<DirectoryInfo, DirectoryMetadata, DirectoryInfoHasher> dirinfo_entry_t;

        static const std::string kClassName;

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        dirinfo_entry_t directory_entry_; // Metadata for cooperation
    };

    // A table maps key into corresponding directory entry
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed, const uint32_t& edge_idx);
        ~DirectoryTable();

        // NOTE: lookup() cannot be const due to rwlock_.try_lock_shared()
        void lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const;
        void update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata);

        uint32_t getSizeForCapacity() const;
    private:
        typedef std::unordered_map<Key, DirectoryEntry, KeyHasher> dirinfo_table_t;

        static const std::string kClassName;

        // Const shared variable
        std::string instance_name_;
        std::mt19937_64* directory_randgen_ptr_;

        // Guarantee the atomicity of directory_hashtable_ (e.g., update dirinfo of different keys)
        mutable Rwlock* rwlock_for_dirtable_ptr_;

        // Non-const shared variable (metadata for cooperation)
        dirinfo_table_t directory_hashtable_; // Maintain directory information (not need ordered map)
    };
}

#endif