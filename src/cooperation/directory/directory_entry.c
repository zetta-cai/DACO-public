#include "cooperation/directory/directory_entry.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryEntry::GET_ALL_DIRINFO_FUNCNAME("getAllDirinfo");
    const std::string DirectoryEntry::GET_ALL_VALID_DIRINFO_FUNCNAME("getAllValidDirinfo");
    const std::string DirectoryEntry::ADD_DIRINFO_FUNCNAME("addDirinfo");
    const std::string DirectoryEntry::REMOVE_DIRINFO_FUNCNAME("removeDirinfo");
    const std::string DirectoryEntry::INVALIDATE_METADATA_FOR_ALL_DIRINFO_IF_EXIST_FUNCNAME("invalidateMetadataForAllDirinfoIfExist");
    const std::string DirectoryEntry::VALIDATE_METADATA_FOR_DIRINFO_IF_EXIST_FUNCNAME("validateMetadataForDirinfoIfExist");

    const std::string DirectoryEntry::kClassName("DirectoryEntry");

    DirectoryEntry::DirectoryEntry()
    {
        directory_entry_.clear();
    }

    DirectoryEntry::~DirectoryEntry() {}

    // (1) Access per-dirinfo metadata

    void DirectoryEntry::getAllDirinfo(dirinfo_set_t& dirinfo_set) const
    {
        dirinfo_set.clear();

        // Add all directory information into directory_info_set
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            const DirectoryInfo& directory_info = iter->first;
            dirinfo_set.insert(directory_info);
        }
        return;
    }

    void DirectoryEntry::getAllValidDirinfo(dirinfo_set_t& dirinfo_set) const
    {
        dirinfo_set.clear();

        // Add all valid directory information into valid_directory_info_set
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            const DirectoryInfo& directory_info = iter->first;
            const DirectoryMetadata& directory_metadata = iter->second;
            if (directory_metadata.isValidMetadata()) // validity = true
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

    void DirectoryEntry::invalidateMetadataForAllDirinfoIfExist(dirinfo_set_t& all_dirinfo)
    {
        for (dirinfo_entry_t::iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            iter->second.invalidateMetadata();
            all_dirinfo.insert(iter->first);
        }
        return;
    }

    void DirectoryEntry::validateMetadataForDirinfoIfExist(const DirectoryInfo& directory_info)
    {
        dirinfo_entry_t::iterator iter = directory_entry_.find(directory_info);
        if (iter != directory_entry_.end())
        {
            iter->second.validateMetadata();
        }
        return;
    }

    // (2) For ConcurrentHashtable

    uint64_t DirectoryEntry::getSizeForCapacity() const
    {
        uint64_t size = 0;
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

        if (function_name == ADD_DIRINFO_FUNCNAME)
        {
            AddDirinfoParam* tmp_param_ptr = static_cast<AddDirinfoParam*>(param_ptr);
            tmp_param_ptr->is_directory_already_exist = addDirinfo(tmp_param_ptr->directory_info, tmp_param_ptr->directory_metadata);
        }
        else if (function_name == REMOVE_DIRINFO_FUNCNAME)
        {
            RemoveDirinfoParam* tmp_param_ptr = static_cast<RemoveDirinfoParam*>(param_ptr);
            tmp_param_ptr->is_directory_already_exist = removeDirinfo(tmp_param_ptr->directory_info);

            if (directory_entry_.size() == 0)
            {
                tmp_param_ptr->is_global_cached = false; // Help DirectoryTable to judge if key is global cached
                is_erase = true; // the key-direntry pair can be erased due to empty direntry
            }
            else
            {
                tmp_param_ptr->is_global_cached = true;
                is_erase = false;
            }
        }
        else if (function_name == INVALIDATE_METADATA_FOR_ALL_DIRINFO_IF_EXIST_FUNCNAME)
        {
            InvalidateMetadataForAllDirinfoIfExistParam* tmp_param_ptr = static_cast<InvalidateMetadataForAllDirinfoIfExistParam*>(param_ptr);
            invalidateMetadataForAllDirinfoIfExist(tmp_param_ptr->all_dirinfo);
        }
        else if (function_name == VALIDATE_METADATA_FOR_DIRINFO_IF_EXIST_FUNCNAME)
        {
            ValidateMetadataForDirinfoIfExistParam* tmp_param_ptr = static_cast<ValidateMetadataForDirinfoIfExistParam*>(param_ptr);
            validateMetadataForDirinfoIfExist(tmp_param_ptr->directory_info);
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

    void DirectoryEntry::constCall(const std::string& function_name, void* param_ptr) const
    {
        if (function_name == GET_ALL_VALID_DIRINFO_FUNCNAME)
        {
            GetAllValidDirinfoParam* tmp_param_ptr = static_cast<GetAllValidDirinfoParam*>(param_ptr);
            getAllValidDirinfo(tmp_param_ptr->dirinfo_set);
        }
        else if (function_name == GET_ALL_DIRINFO_FUNCNAME)
        {
            GetAllDirinfoParam* tmp_param_ptr = static_cast<GetAllDirinfoParam*>(param_ptr);
            getAllDirinfo(tmp_param_ptr->dirinfo_set);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for constCall()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        
        return;
    }

    const DirectoryEntry& DirectoryEntry::operator=(const DirectoryEntry& other)
    {
        directory_entry_ = other.directory_entry_;
        return *this;
    }
}