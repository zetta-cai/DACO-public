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

#include "common/key.h"
#include "cooperation/directory_info.h"

namespace covered
{
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed);
        ~DirectoryTable();

        void lookup(const Key& key, bool& is_directory_exist, DirectoryInfo& directory_info) const;
        void update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);
    private:
        // TODO: use std::atomic<bool> if edge node has multiple sub-threads (now each edge node only has 1 thread to process data/control requests)
        typedef std::unordered_map<DirectoryInfo, bool, DirectoryInfoHasher> perkey_dirinfos_t;
        typedef std::unordered_map<Key, perkey_dirinfos_t, KeyHasher> dirinfo_table_t;
        
        static const std::string kClassName;

        dirinfo_table_t directory_hashtable_; // Maintain directory information (not need ordered map)
        std::mt19937_64* directory_randgen_ptr_;
    };
}

#endif