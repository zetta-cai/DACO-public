/*
 * DirectoryTable: manage directory information of different keys (thread safe).
 *
 * NOTE: all non-const shared variables in DirectoryTable should be thread safe.
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
#include "concurrency/concurrent_hashtable_impl.h"
#include "cooperation/directory/directory_entry.h"

namespace covered
{
    // A table maps key into corresponding directory entry
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed, const uint32_t& edge_idx);
        ~DirectoryTable();

        void lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const;
        void update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata);
        bool isCooperativeCached(const Key& key) const;
        void invalidateAllDirinfo(const Key& key, dirinfo_set_t& all_dirinfo);
        void validateDirinfoIfExist(const Key& key, const DirectoryInfo& directory_info);

        uint32_t getSizeForCapacity() const;
    private:
        typedef ConcurrentHashtable<DirectoryEntry, KeyHasher> dirinfo_table_t;

        static const std::string kClassName;

        // Const shared variable
        std::string instance_name_;
        std::mt19937_64* directory_randgen_ptr_;

        // Non-const shared variable (metadata for cooperation)
        dirinfo_table_t directory_hashtable_; // Maintain directory information (not need ordered map)
    };
}

#endif