#include "cooperation/directory/directory_entry.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryEntry::kClassName("DirectoryEntry");

    DirectoryEntry::DirectoryEntry()
    {
        directory_entry_.clear();
    }

    DirectoryEntry::~DirectoryEntry() {}

    void DirectoryEntry::getValidDirinfoSet(dirinfo_set_t& dirinfo_set) const
    {
        dirinfo_set.clear();

        // Add all valid directory information into valid_directory_info_set
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            const DirectoryInfo& directory_info = iter->first;
            const DirectoryMetadata& directory_metadata = iter->second;
            if (directory_metadata.isValidDirinfo()) // validity = true
            {
                dirinfo_set.insert(directory_info);
            }
        }
        return;
    }

    bool DirectoryEntry::addDirinfo(const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata)
    {
        bool is_directory_already_exist = false;
        dirinfo_entry_t::iterator iter = directory_entry_.find(directory_info);
        if (iter == directory_entry_.end()) // directory info does not exist
        {
            directory_entry_.insert(std::pair<DirectoryInfo, DirectoryMetadata>(directory_info, directory_metadata));

            is_directory_already_exist = false;
        }
        else // directory info already exists
        {
            iter->second = directory_metadata;

            is_directory_already_exist = true;
        }
        return is_directory_already_exist;
    }

    bool DirectoryEntry::removeDirinfo(const DirectoryInfo& directory_info)
    {
        bool is_directory_already_exist = false;
        dirinfo_entry_t::const_iterator iter = directory_entry_.find(directory_info);
        if (iter == directory_entry_.end()) // directory info does not exist
        {
            is_directory_already_exist = false;
        }
        else // directory info already exists
        {
            directory_entry_.erase(iter);

            is_directory_already_exist = true;
        }
        return is_directory_already_exist;
    }

    void DirectoryEntry::invalidateDirentry(dirinfo_set_t& all_dirinfo)
    {
        for (dirinfo_entry_t::iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            iter->second.invalidateDirinfo();
            all_dirinfo.insert(iter->first);
        }
        return;
    }

    DirectoryMetadata* DirectoryEntry::getDirectoryMetadataPtr(const DirectoryInfo& directory_info)
    {
        DirectoryMetadata* directory_metadata_ptr = NULL;
        dirinfo_entry_t::iterator iter = directory_entry_.find(directory_info);
        if (iter != directory_entry_.end())
        {
            directory_metadata_ptr = &(iter->second);
        }
        return directory_metadata_ptr;
    }

    uint32_t DirectoryEntry::getSizeForCapacity() const
    {
        uint32_t size = 0;
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            size += iter->first.getSizeForCapacity();
            size += iter->second.getSizeForCapacity();
        }
        return size;
    }

    bool DirectoryEntry::call(const std::string& function_name, void* param_ptr)
    {
        assert(param_ptr != NULL);

        bool is_erase = false;

        if (function_name == "addDirinfo")
        {
            AddDirinfoParam* tmp_param_ptr = static_cast<AddDirinfoParam*>(param_ptr);
            tmp_param_ptr->is_directory_already_exist = addDirinfo(tmp_param_ptr->directory_info, tmp_param_ptr->directory_metadata);
        }
        else if (function_name == "removeDirinfo")
        {
            RemoveDirinfoParam* tmp_param_ptr = static_cast<RemoveDirinfoParam*>(param_ptr);
            tmp_param_ptr->is_directory_already_exist = removeDirinfo(tmp_param_ptr->directory_info);

            if (directory_entry_.size() == 0)
            {
                is_erase = true; // the key-direntry pair can be erased due to empty direntry
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for call()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return is_erase;
    }

    DirectoryEntry& DirectoryEntry::operator=(const DirectoryEntry& other)
    {
        directory_entry_ = other.directory_entry_;
        return *this;
    }
}