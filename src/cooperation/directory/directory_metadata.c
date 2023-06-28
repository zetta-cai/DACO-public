#include "cooperation/directory/directory_metadata.h"

namespace covered
{
    const std::string DirectoryMetadata::kClassName("DirectoryMetadata");

    DirectoryMetadata::DirectoryMetadata(const bool& is_valid)
    {
        is_valid_ = is_valid;
    }

    DirectoryMetadata::~DirectoryMetadata() {}

    bool DirectoryMetadata::isValidDirinfo() const
    {
        return is_valid_;
    }

    void DirectoryMetadata::validateDirinfo()
    {
        is_valid_ = true;
    }
    
    void DirectoryMetadata::invalidateDirinfo()
    {
        is_valid_ = false;
    }

    uint32_t DirectoryMetadata::getSizeForCapacity() const
    {
        return sizeof(bool);
    }

    DirectoryMetadata& DirectoryMetadata::operator=(const DirectoryMetadata& other)
    {
        is_valid_ = other.is_valid_;
        return *this;
    }

    

}