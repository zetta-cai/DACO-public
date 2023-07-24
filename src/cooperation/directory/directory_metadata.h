/*
 * DirectoryMetadata: directory metadata for DHT-based content discovery (building blocks for DirectoryTable).
 * 
 * By Siyuan Sheng (2023.06.28).
 */

#ifndef DIRECTORY_METADATA_H
#define DIRECTORY_METADATA_H

#include <string>

namespace covered
{
    class DirectoryMetadata
    {
    public:
        DirectoryMetadata(const bool& is_valid);
        ~DirectoryMetadata();

        bool isValidMetadata() const;
        void validateMetadata();
        void invalidateMetadata();

        uint64_t getSizeForCapacity() const;

        const DirectoryMetadata& operator=(const DirectoryMetadata& other);
    private:
        static const std::string kClassName;

        // NOTE: conflictions between reader(s) and write(s) have been fixed by DirectoryTable::rwlock_
        bool is_valid_; // Metadata for cooperation
    };
}

#endif