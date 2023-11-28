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
#include "concurrency/perkey_rwlock.h"
#include "cooperation/directory/directory_entry.h"
#include "cooperation/directory/dirinfo_set.h"
#include "cooperation/directory/metadata_update_requirement.h"

namespace covered
{
    // A table maps key into corresponding directory entry
    class DirectoryTable
    {
    public:
        DirectoryTable(const uint32_t& seed, const uint32_t& edge_idx, const PerkeyRwlock* perkey_rwlock_ptr);
        ~DirectoryTable();

        DirinfoSet getAll(const Key& key) const;

        // Return whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        bool lookup(const Key& key, const uint32_t& source_edge_idx, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const; // Find a non-source valid directory info if any
        bool update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata, MetadataUpdateRequirement& metadata_update_requirement);
        bool isGlobalCached(const Key& key) const;
        bool isCachedByGivenEdge(const Key& key, const uint32_t& edge_idx) const;
        bool isNeighborCached(const Key& key, const uint32_t& edge_idx) const;

        void invalidateAllDirinfoForKeyIfExist(const Key& key, DirinfoSet& all_dirinfo); // Invalidate all dirinfos only if key exists (NOT add an empty direntry)
        void validateDirinfoForKeyIfExist(const Key& key, const DirectoryInfo& directory_info); // Validate only if key and dirinfo exist (NOT add a valid dirinfo)

        uint64_t getSizeForCapacity() const;
    private:
        typedef ConcurrentHashtable<DirectoryEntry> dirinfo_table_t;

        static const std::string kClassName;

        // Const shared variable
        std::string instance_name_;
        std::mt19937_64* directory_randgen_ptr_;

        // Non-const shared variable (metadata for cooperation)
        dirinfo_table_t directory_hashtable_; // Maintain directory information (thread safe)
    };
}

#endif