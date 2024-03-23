#include "cooperation/directory/directory_metadata.h"

#include <assert.h>

namespace covered
{
    const std::string DirectoryMetadata::kClassName("DirectoryMetadata");

    DirectoryMetadata::DirectoryMetadata()
    {
        is_valid_ = false;
    }

    DirectoryMetadata::DirectoryMetadata(const bool& is_valid)
    {
        is_valid_ = is_valid;
    }

    DirectoryMetadata::~DirectoryMetadata() {}

    bool DirectoryMetadata::isValidMetadata() const
    {
        return is_valid_;
    }

    void DirectoryMetadata::validateMetadata()
    {
        is_valid_ = true;
    }
    
    void DirectoryMetadata::invalidateMetadata()
    {
        is_valid_ = false;
    }

    uint64_t DirectoryMetadata::getSizeForCapacity() const
    {
        return sizeof(bool);
    }

    const DirectoryMetadata& DirectoryMetadata::operator=(const DirectoryMetadata& other)
    {
        is_valid_ = other.is_valid_;
        return *this;
    }

    // Dump/load directory metadata of directory entry of directory table for cooperation snapshot

    void DirectoryMetadata::dumpDirectoryMetadata(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        fs_ptr->write((const char*)&is_valid_, sizeof(bool));

        return;
    }

    void DirectoryMetadata::loadDirectoryMetadata(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        fs_ptr->read((char*)&is_valid_, sizeof(bool));

        return;
    }

}