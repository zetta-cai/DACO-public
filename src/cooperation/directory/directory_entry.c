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

    void DirectoryEntry::getAllDirinfo(DirinfoSet& dirinfo_set) const
    {
        std::list<DirectoryInfo> tmp_dirinfo_set;
        tmp_dirinfo_set.clear();

        // Add all directory information into directory_info_set
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            const DirectoryInfo& directory_info = iter->first;
            tmp_dirinfo_set.push_back(directory_info);
        }

        dirinfo_set = DirinfoSet(tmp_dirinfo_set);
        return;
    }

    void DirectoryEntry::getAllValidDirinfo(DirinfoSet& dirinfo_set) const
    {
        std::list<DirectoryInfo> tmp_dirinfo_set;
        tmp_dirinfo_set.clear();

        // Add all valid directory information into valid_directory_info_set
        for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            const DirectoryInfo& directory_info = iter->first;
            const DirectoryMetadata& directory_metadata = iter->second;
            if (directory_metadata.isValidMetadata()) // validity = true
            {
                tmp_dirinfo_set.push_back(directory_info);
            }
        }

        dirinfo_set = DirinfoSet(tmp_dirinfo_set);
        return;
    }

    bool DirectoryEntry::addDirinfo(const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata, MetadataUpdateRequirement& metadata_update_requirement)
    {
        bool is_directory_already_exist = false;
        dirinfo_entry_t::iterator iter = directory_entry_.find(directory_info);
        if (iter == directory_entry_.end()) // directory info does not exist
        {
            const bool is_from_single_to_multiple = (directory_entry_.size() == 1);
            const bool is_from_multiple_to_single = false; // Must NOT multiple-to-single due to NOT directory eviction
            uint32_t notify_edge_idx = 0; // The edge node index of the first cache copy
            if (is_from_single_to_multiple)
            {
                notify_edge_idx = directory_entry_.begin()->first.getTargetEdgeIdx();
            }

            directory_entry_.insert(std::pair<DirectoryInfo, DirectoryMetadata>(directory_info, directory_metadata));

            is_directory_already_exist = false;

            if (is_from_single_to_multiple) // Need to notify the first cache copy for single-to-multiple
            {
                metadata_update_requirement = MetadataUpdateRequirement(is_from_single_to_multiple, is_from_multiple_to_single, notify_edge_idx);
            }
            else // NO need to notify the first cache copy for empty-to-single or multiple-to-multiple
            {
                metadata_update_requirement = MetadataUpdateRequirement();
            }
        }
        else // directory info already exists
        {
            iter->second = directory_metadata;

            is_directory_already_exist = true;

            metadata_update_requirement = MetadataUpdateRequirement(); // No need to notify the first cache copy on metadata update for single-to-multiple due to NOT admiting a new dirinfo
        }
        return is_directory_already_exist;
    }

    bool DirectoryEntry::removeDirinfo(const DirectoryInfo& directory_info, MetadataUpdateRequirement& metadata_update_requirement)
    {
        bool is_directory_already_exist = false;
        dirinfo_entry_t::const_iterator iter = directory_entry_.find(directory_info);
        if (iter == directory_entry_.end()) // directory info does not exist
        {
            is_directory_already_exist = false;

            metadata_update_requirement = MetadataUpdateRequirement(); // NO need to notify the last cache copy on metadata update for multiple-to-single due to NOT evicting an existing dirinfo
        }
        else // directory info already exists
        {
            directory_entry_.erase(iter);

            const bool is_single_to_multiple = false; // Must NOT single-to-multiple due to evicting instead of admiting dirinfo
            const bool is_multiple_to_single = (directory_entry_.size() == 1);
            uint32_t notify_edge_idx = 0; // The edge node index of the last cache copy
            if (is_multiple_to_single)
            {
                notify_edge_idx = directory_entry_.begin()->first.getTargetEdgeIdx();
            }

            is_directory_already_exist = true;

            if (is_multiple_to_single) // Need to notify the last cache copy for multiple-to-single
            {
                metadata_update_requirement = MetadataUpdateRequirement(is_single_to_multiple, is_multiple_to_single, notify_edge_idx);
            }
            else // NO need to notify the last cache copy for multiple-to-multiple or single-to-empty
            {
                metadata_update_requirement = MetadataUpdateRequirement();
            }
        }
        return is_directory_already_exist;
    }

    void DirectoryEntry::invalidateMetadataForAllDirinfoIfExist(DirinfoSet& all_dirinfo)
    {
        std::list<DirectoryInfo> tmp_dirinfo_set;
        for (dirinfo_entry_t::iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
        {
            iter->second.invalidateMetadata();
            tmp_dirinfo_set.push_back(iter->first);
        }
        all_dirinfo = DirinfoSet(tmp_dirinfo_set);
        return;
    }

    bool DirectoryEntry::validateMetadataForDirinfoIfExist(const DirectoryInfo& directory_info)
    {
        bool is_dirinfo_exist = false;
        dirinfo_entry_t::iterator iter = directory_entry_.find(directory_info);
        if (iter != directory_entry_.end())
        {
            iter->second.validateMetadata();
            is_dirinfo_exist = true;
        }
        return is_dirinfo_exist;
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
            tmp_param_ptr->is_directory_already_exist = addDirinfo(tmp_param_ptr->directory_info, tmp_param_ptr->directory_metadata, tmp_param_ptr->metadata_update_requirement_ref);
        }
        else if (function_name == REMOVE_DIRINFO_FUNCNAME)
        {
            RemoveDirinfoParam* tmp_param_ptr = static_cast<RemoveDirinfoParam*>(param_ptr);
            tmp_param_ptr->is_directory_already_exist = removeDirinfo(tmp_param_ptr->directory_info, tmp_param_ptr->metadata_update_requirement_ref);

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
            tmp_param_ptr->is_dirinfo_exist = validateMetadataForDirinfoIfExist(tmp_param_ptr->directory_info);
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

    // (3) Dump/load directory entry for directory table of cooperation snapshot

    void DirectoryEntry::dumpDirectoryEntry(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Dump dirinfo-dirmetadata pairs
        // (1) pair cnt
        const uint32_t pair_cnt = directory_entry_.size();
        fs_ptr->write((const char*)&pair_cnt, sizeof(uint32_t));
        // (2) dirinfo-dirmetadata pairs
        for (dirinfo_entry_t::const_iterator tmp_const_iter = directory_entry_.begin(); tmp_const_iter != directory_entry_.end(); tmp_const_iter++)
        {
            // Dump the dirinfo
            const DirectoryInfo& directory_info = tmp_const_iter->first;
            directory_info.serialize(fs_ptr);

            // Dump the directory metadata
            const DirectoryMetadata& directory_metadata = tmp_const_iter->second;
            directory_metadata.dumpDirectoryMetadata(fs_ptr);
        }

        return;
    }

    void DirectoryEntry::loadDirectoryEntry(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load dirinfo-dirmetadata pairs
        // (1) pair cnt
        uint32_t pair_cnt = 0;
        fs_ptr->read((char*)&pair_cnt, sizeof(uint32_t));
        // (2) dirinfo-dirmetadata pairs
        directory_entry_.clear();
        for (uint32_t i = 0; i < pair_cnt; i++)
        {
            // Load the dirinfo
            DirectoryInfo directory_info;
            directory_info.deserialize(fs_ptr);

            // Load the directory metadata
            DirectoryMetadata directory_metadata;
            directory_metadata.loadDirectoryMetadata(fs_ptr);

            // Update the directory entry
            directory_entry_.insert(std::pair<DirectoryInfo, DirectoryMetadata>(directory_info, directory_metadata));
        }

        return;
    }
}