/*
 * DirectoryEntry: a directory entry stores multiple directory infos and metadatas for the given key (building blocks for DirectoryTable).
 * 
 * By Siyuan Sheng (2023.06.28).
 */

#ifndef DIRECTORY_ENTRY_H
#define DIRECTORY_ENTRY_H

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "cooperation/directory/directory_info.h"
#include "cooperation/directory/directory_metadata.h"

namespace covered
{
    class DirectoryEntry
    {
    public:
        struct AddDirinfoParam
        {
            const DirectoryInfo& directory_info;
            const DirectoryMetadata& directory_metadata;
            bool is_directory_already_exist;
        };
        
        struct RemoveDirinfoParam
        {
            const DirectoryInfo& directory_info;
            bool is_directory_already_exist;
        };

        struct InvalidateDirentryParam
        {
            dirinfo_set_t& all_dirinfo;
        };

        struct GetDirectoryMetadataPtrParam
        {
            const DirectoryInfo& directory_info;
            DirectoryMetadata* directory_metadata_ptr;
        };

        DirectoryEntry();
        ~DirectoryEntry();

        void getValidDirinfoSet(dirinfo_set_t& dirinfo_set) const;
        bool addDirinfo(const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata); // return is_directory_already_exist
        bool removeDirinfo(const DirectoryInfo& directory_info); // return is_directory_already_exist
        void invalidateDirentry(dirinfo_set_t& all_dirinfo);
        DirectoryMetadata* getDirectoryMetadataPtr(const DirectoryInfo& directory_info);

        uint32_t getSizeForCapacity() const;
        bool call(const std::string& function_name, void* param_ptr);
        void constCall(const std::string& function_name, void* param_ptr) const;

        DirectoryEntry& operator=(const DirectoryEntry& other);
    private:
        typedef std::unordered_map<DirectoryInfo, DirectoryMetadata, DirectoryInfoHasher> dirinfo_entry_t;

        static const std::string kClassName;

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        dirinfo_entry_t directory_entry_; // Metadata for cooperation
    };
}

#endif