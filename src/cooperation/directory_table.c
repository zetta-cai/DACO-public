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

    DirinfoSet DirectoryTable::getAll(const Key& key) const
    {
        DirinfoSet all_dirinfo = DirinfoSet(std::list<DirectoryInfo>());

        // Prepare GetAllDirinfoParam
        DirectoryEntry::GetAllDirinfoParam tmp_param = {all_dirinfo};

        bool is_exist = false;
        directory_hashtable_.constCallIfExist(key, is_exist, DirectoryEntry::GET_ALL_DIRINFO_FUNCNAME, &tmp_param); // Get directory entry if key exists

        if (!is_exist) // key does not exist
        {
            assert(!all_dirinfo.isInvalid()); // Still valid DirinfoSet with an empty unordered set
        }
        else // key (i.e., directory entry) exists
        {
            uint32_t tmp_size = 0;
            assert(all_dirinfo.getDirinfoSetSizeIfComplete(tmp_size)); // dirinfo set from directory entry MUST be complete
            assert(tmp_size > 0); // Directory entry MUST NOT be empty
        }

        return all_dirinfo;
    }

    bool DirectoryTable::lookup(const Key& key, const uint32_t& source_edge_idx, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        bool is_global_cached = false; // Whether the key is cached by a local/neighbor edge node (even if invalid temporarily)

        // Prepare GetAllValidDirinfoParam
        DirinfoSet valid_directory_info_set;
        DirectoryEntry::GetAllValidDirinfoParam tmp_param = {valid_directory_info_set};

        bool is_exist = false;
        uint32_t dirinfo_set_size = 0;
        directory_hashtable_.constCallIfExist(key, is_exist, DirectoryEntry::GET_ALL_VALID_DIRINFO_FUNCNAME, &tmp_param); // Get directory entry if key exists
        if (!is_exist) // key does not exist
        {
            is_valid_directory_exist = false;
            is_global_cached = false;
        }
        else // key exists
        {
            // Remove source edge idx from valid_directory_info_set if any due to finding a non-source valid dirinfo
            bool is_erase = false;
            bool with_complete_dirinfo_set = valid_directory_info_set.tryToEraseIfComplete(DirectoryInfo(source_edge_idx), is_erase);
            assert(with_complete_dirinfo_set); // dirinfo set from directory entry MUST be complete
            UNUSED(is_erase);

            with_complete_dirinfo_set = valid_directory_info_set.getDirinfoSetSizeIfComplete(dirinfo_set_size);
            assert(with_complete_dirinfo_set); // dirinfo set from directory entry MUST be complete
            if (dirinfo_set_size > 0) // At least one valid directory
            {
                is_valid_directory_exist = true;
            }
            else // No valid directory (e.g., key is being written)
            {
                is_valid_directory_exist = false;
            }
            is_global_cached = true;
        } // End of if key exists

        // Get the target edge index from valid neighbors
        if (is_valid_directory_exist)
        {
            assert(dirinfo_set_size > 0); // At least one valid dirinfo

            // Randomly select a valid edge node as the target edge node
            std::uniform_int_distribution<uint32_t> uniform_dist(0, dirinfo_set_size - 1); // Range of [0, # of directory info - 1]
            uint32_t random_number = uniform_dist(*directory_randgen_ptr_);
            assert(random_number < dirinfo_set_size);
            bool with_complete_dirinfo_set = valid_directory_info_set.getDirinfoIfComplete(random_number, directory_info);
            assert(with_complete_dirinfo_set); // NOTE: dirinfo set from directory entry MUST be complete
            assert(directory_info.getTargetEdgeIdx() != source_edge_idx); // NOTE: find a non-source valid directory info if any
        }

        return is_global_cached;
    }

    bool DirectoryTable::update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata, MetadataUpdateRequirement& metadata_update_requirement)
    {
        bool is_global_cached = false; // Whether the key is cached by a local/neighbor edge node (even if invalid temporarily)

        if (is_admit) // Add a new directory info
        {
            // Prepare directory entry for the key
            DirectoryEntry directory_entry;
            MetadataUpdateRequirement unused_metadata_update_requirement; // Empty-to-single
            bool tmp_is_directory_already_exist = directory_entry.addDirinfo(directory_info, directory_metadata, unused_metadata_update_requirement);
            UNUSED(unused_metadata_update_requirement);
            assert(tmp_is_directory_already_exist == false);

            // Prepare AddDirinfoParam
            DirectoryEntry::AddDirinfoParam tmp_param = {directory_info, directory_metadata, false, metadata_update_requirement};

            // Insert a new directory entry, or add directory info into existing directory entry
            bool is_exist = false;
            directory_hashtable_.insertOrCall(key, directory_entry, is_exist, DirectoryEntry::ADD_DIRINFO_FUNCNAME, (void*)&tmp_param);
            if (!is_exist) // key does not exist
            {
                // NOTE: maybe invalid directory info if key is being written
                //assert(directory_metadata.isValidDirinfo());

                metadata_update_requirement = MetadataUpdateRequirement(); // NO need to notify the first cache copy on metadata update for first-to-multiple due to empty-to-first
            }
            else // key already exists
            {
                if (tmp_param.is_directory_already_exist) // directory_info should NOT exist for key
                {                    
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " already exists for key " << key.getKeyDebugstr() << " in update() with is_admit = true!";
                    Util::dumpWarnMsg(instance_name_, oss.str());
                }
            }

            is_global_cached = true; // Key MUST be global cached after inserting a new content directory
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            // Prepare RemoveDirinfoParam
            DirectoryEntry::RemoveDirinfoParam tmp_param = {directory_info, false, false, metadata_update_requirement};

            bool is_exist = false;
            directory_hashtable_.callIfExist(key, is_exist, DirectoryEntry::REMOVE_DIRINFO_FUNCNAME, &tmp_param);
            if (is_exist) // key already exists
            {
                if (!tmp_param.is_directory_already_exist) // directory_info should exist for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " does NOT exist for key " << key.getKeyDebugstr() << " in update() with is_admit = false!";
                    Util::dumpWarnMsg(instance_name_, oss.str());
                }

                is_global_cached = tmp_param.is_global_cached;
            }
            else // key does NOT exist
            {
                std::ostringstream oss;
                oss << "key " << key.getKeyDebugstr() << " does not exist in directory_hashtable_ in update() with is_admit = false!";
                Util::dumpWarnMsg(instance_name_, oss.str());

                metadata_update_requirement = MetadataUpdateRequirement(); // NO need to notify the last cache copy on metadata update for multiple-to-first due to empty-to-empty

                is_global_cached = false;
            }
        } // ENd of (is_admit == false)

        return is_global_cached;
    }

    bool DirectoryTable::isGlobalCached(const Key& key) const
    {
        bool is_global_cached = directory_hashtable_.isExist(key); // Whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        return is_global_cached;
    }

    bool DirectoryTable::isCachedByGivenEdge(const Key& key, const uint32_t& edge_idx) const
    {
        bool is_cached_by_given_edge = false;

        DirinfoSet all_dirinfo = getAll(key);
        bool with_complete_dirinfo_set = all_dirinfo.isExistIfComplete(DirectoryInfo(edge_idx), is_cached_by_given_edge);
        assert(with_complete_dirinfo_set); // dirinfo set from directory entry MUST be complete

        return is_cached_by_given_edge;
    }

    bool DirectoryTable::isNeighborCached(const Key& key, const uint32_t& edge_idx) const
    {
        bool is_neighbor_cached = false;

        DirinfoSet all_dirinfo = getAll(key);
        std::list<DirectoryInfo> dirinfo_list;
        bool with_complete_dirinfo_set = all_dirinfo.getDirinfoSetIfComplete(dirinfo_list);
        assert(with_complete_dirinfo_set); // dirinfo set from directory entry MUST be complete

        // Check if any neighbor edge node (NOT the current node with the given edge_idx) caches the object
        if (dirinfo_list.size() > 0)
        {
            for (std::list<DirectoryInfo>::const_iterator dirinfo_const_iter = dirinfo_list.begin(); dirinfo_const_iter != dirinfo_list.end(); dirinfo_const_iter++)
            {
                if (dirinfo_const_iter->getTargetEdgeIdx() != edge_idx)
                {
                    is_neighbor_cached = true;
                    break;
                }
            }
        }

        return is_neighbor_cached;
    }

    void DirectoryTable::invalidateAllDirinfoForKeyIfExist(const Key& key, DirinfoSet& all_dirinfo)
    {
        // Prepare InvalidateMetadataForAllDirinfoParam
        DirectoryEntry::InvalidateMetadataForAllDirinfoIfExistParam tmp_param = {all_dirinfo};

        bool is_exist = false;
        directory_hashtable_.callIfExist(key, is_exist, DirectoryEntry::INVALIDATE_METADATA_FOR_ALL_DIRINFO_IF_EXIST_FUNCNAME, &tmp_param);
        
        assert(is_exist == true); // NOTE: invalidateAllDirinfoForKeyIfExist() is invoked ONLY if key exists

        uint32_t tmp_size = 0;
        bool with_complete_dirinfo_set = all_dirinfo.getDirinfoSetSizeIfComplete(tmp_size);
        assert(with_complete_dirinfo_set); // dirinfo set from directory entry MUST be complete
        assert(tmp_size > 0); // Directory entry MUST NOT be empty

        return;
    }

    void DirectoryTable::validateDirinfoForKeyIfExist(const Key& key, const DirectoryInfo& directory_info, bool& is_key_exist, bool& is_dirinfo_exist)
    {
        // Prepare ValidateMetadataForDirinfoIfExistParam
        DirectoryEntry::ValidateMetadataForDirinfoIfExistParam tmp_param = {directory_info};

        is_key_exist = false;
        is_dirinfo_exist = false;
        directory_hashtable_.callIfExist(key, is_key_exist, DirectoryEntry::VALIDATE_METADATA_FOR_DIRINFO_IF_EXIST_FUNCNAME, &tmp_param);
        if (is_key_exist)
        {
            is_dirinfo_exist = tmp_param.is_dirinfo_exist;
        }

        return;
    }

    uint64_t DirectoryTable::getSizeForCapacity() const
    {
        // NOTE: note that CacheWrapperBase counts the size of keys cached for local edge cache as closest edge node, so we still need to count the size of keys managed for cooperation as beacon edge node
        // NOTE: as we have counted the size of keys for cooperation, other structures (e.g., BlockTracker) does NOT need to count key sizes again
        uint64_t total_key_size = directory_hashtable_.getTotalKeySizeForCapcity();
        uint64_t total_value_size = directory_hashtable_.getTotalValueSizeForCapcity();
        uint64_t size = Util::uint64Add(total_key_size, total_value_size);

        return size;
    }
}