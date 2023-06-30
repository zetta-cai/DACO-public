#include "cooperation/directory_table.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryTable::kClassName("DirectoryTable");

    DirectoryTable::DirectoryTable(const uint32_t& seed, const uint32_t& edge_idx, const PerkeyRwlock* perkey_rwlock_ptr) : directory_hashtable_("directory_hashtable_", DirectoryEntry(), perkey_rwlock_ptr)
    {
        // Allocate randgen to choose target edge node from directory information
        directory_randgen_ptr_ = new std::mt19937_64(seed);
        assert(directory_randgen_ptr_ != NULL);

        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    DirectoryTable::~DirectoryTable()
    {
        // Release randgen
        assert(directory_randgen_ptr_ != NULL);
        delete directory_randgen_ptr_;
        directory_randgen_ptr_ = NULL;
    }

    void DirectoryTable::lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // Prepare GetAllValidDirinfoParam
        dirinfo_set_t valid_directory_info_set;
        DirectoryEntry::GetAllValidDirinfoParam tmp_param = {valid_directory_info_set};

        bool is_exist = false;
        directory_hashtable_.constCallIfExist(key, is_exist, DirectoryEntry::GET_ALL_VALID_DIRINFO_FUNCNAME, &tmp_param); // Get directory entry if key exists
        if (!is_exist) // key does not exist
        {
            is_valid_directory_exist = false;
        }
        else // key exists
        {
            if (valid_directory_info_set.size() > 0) // At least one valid directory
            {
                is_valid_directory_exist = true;
            }
            else // No valid directory (e.g., key is being written)
            {
                is_valid_directory_exist = false;
            }
        } // End of if key exists

        // Get the target edge index from valid neighbors
        if (is_valid_directory_exist)
        {
            // Randomly select a valid edge node as the target edge node
            std::uniform_int_distribution<uint32_t> uniform_dist(0, valid_directory_info_set.size() - 1); // Range from 0 to (# of directory info - 1)
            uint32_t random_number = uniform_dist(*directory_randgen_ptr_);
            assert(random_number < valid_directory_info_set.size());
            uint32_t i = 0;
            for (dirinfo_set_t::const_iterator iter = valid_directory_info_set.begin(); iter != valid_directory_info_set.end(); iter++)
            {
                if (i == random_number)
                {
                    directory_info = *iter;
                    break;
                }
                i++;
            }
        }

        return;
    }

    void DirectoryTable::update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata)
    {
        if (is_admit) // Add a new directory info
        {
            // Prepare directory entry for the key
            DirectoryEntry directory_entry;
            bool tmp_is_directory_already_exist = directory_entry.addDirinfo(directory_info, directory_metadata);
            assert(tmp_is_directory_already_exist == false);

            // Prepare AddDirinfoParam
            DirectoryEntry::AddDirinfoParam tmp_param = {directory_info, directory_metadata, false};

            // Insert a new directory entry, or add directory info into existing directory entry
            bool is_exist = false;
            directory_hashtable_.insertOrCall(key, directory_entry, is_exist, DirectoryEntry::ADD_DIRINFO_FUNCNAME, (void*)&tmp_param);
            if (!is_exist) // key does not exist
            {
                //assert(directory_metadata.isValidDirinfo()); // invalid directory info if key is being written
            }
            else // key already exists
            {
                if (tmp_param.is_directory_already_exist) // directory_info should NOT exist for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " already exists for key " << key.getKeystr() << " in update() with is_admit = true!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
            }
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            // Prepare RemoveDirinfoParam
            DirectoryEntry::RemoveDirinfoParam tmp_param = {directory_info, false};

            bool is_exist = false;
            directory_hashtable_.callIfExist(key, is_exist, DirectoryEntry::REMOVE_DIRINFO_FUNCNAME, &tmp_param);
            if (is_exist) // key already exists
            {
                if (!tmp_param.is_directory_already_exist) // directory_info should exist for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " does NOT exist for key " << key.getKeystr() << " in update() with is_admit = false!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
            }
            else // key does NOT exist
            {
                std::ostringstream oss;
                oss << "key " << key.getKeystr() << " does not exist in directory_hashtable_ in update() with is_admit = false!";
                Util::dumpWarnMsg(kClassName, oss.str());
            }
        } // ENd of (is_admit == false)

        return;
    }

    bool DirectoryTable::isCooperativeCached(const Key& key) const
    {
        bool is_cooperative_cached = directory_hashtable_.isExist(key);
        return is_cooperative_cached;
    }

    void DirectoryTable::invalidateAllDirinfoForKeyIfExist(const Key& key, dirinfo_set_t& all_dirinfo)
    {
        // Prepare InvalidateMetadataForAllDirinfoParam
        DirectoryEntry::InvalidateMetadataForAllDirinfoIfExistParam tmp_param = {all_dirinfo};

        bool is_exist = false;
        directory_hashtable_.callIfExist(key, is_exist, DirectoryEntry::INVALIDATE_METADATA_FOR_ALL_DIRINFO_IF_EXIST_FUNCNAME, &tmp_param);
        UNUSED(is_exist);

        return;
    }

    void DirectoryTable::validateDirinfoForKeyIfExist(const Key& key, const DirectoryInfo& directory_info)
    {
        // Prepare ValidateMetadataForDirinfoIfExistParam
        DirectoryEntry::ValidateMetadataForDirinfoIfExistParam tmp_param = {directory_info};

        bool is_exist = false;
        directory_hashtable_.callIfExist(key, is_exist, DirectoryEntry::VALIDATE_METADATA_FOR_DIRINFO_IF_EXIST_FUNCNAME, &tmp_param);
        UNUSED(is_exist);

        return;
    }

    uint32_t DirectoryTable::getSizeForCapacity() const
    {
        // NOTE: note that CacheWrapperBase counts the size of keys cached for local edge cache as closest edge node, so we still need to count the size of keys managed for cooperation as beacon edge node
        // NOTE: as we have counted the size of keys for cooperation, other structures (e.g., BlockTracker) does NOT need to count key sizes again
        uint32_t size = directory_hashtable_.getTotalKeySizeForCapcity() + directory_hashtable_.getTotalValueSizeForCapcity();

        return size;
    }
}