#include "cooperation/directory_table.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryMetadata::kClassName("DirectoryMetadata");
    const std::string DirectoryEntry::kClassName("DirectoryEntry");
    const std::string DirectoryTable::kClassName("DirectoryTable");

    // (1) DirectoryMetadata

    DirectoryMetadata::DirectoryMetadata(const bool& is_valid)
    {
        is_valid_ = is_valid;
    }

    DirectoryMetadata::~DirectoryMetadata() {}

    bool DirectoryMetadata::isValid() const
    {
        return is_valid_;
    }

    void DirectoryMetadata::validate()
    {
        is_valid_ = true;
    }
    
    void DirectoryMetadata::invalidate()
    {
        is_valid_ = false;
    }

    // (2) DirectoryEntry

    DirectoryEntry::DirectoryEntry()
    {
        directory_entry_.clear();
    }

    DirectoryEntry::~DirectoryEntry() {}

    bool DirectoryEntry::isBeingWritten()
    {
        bool is_being_written = false;
        if (rwlock_.try_lock_shared()) // Being able to get read lock means no writes
        {
            is_being_written = false;
            rwlock_.unlock_shared();
        }
        else // Unable to get read lock means with writes
        {
            is_being_written = true;
        }
        return is_being_written;
    }

    bool DirectoryEntry::getValidDirinfoSet(dirinfo_set_t& dirinfo_set)
    {
        bool is_being_written = false;
        if (rwlock_.try_lock_shared()) // Being able to get read lock means no writes
        {
            is_being_written = false;

            // Add all valid directory information into valid_directory_info_set
            for (dirinfo_entry_t::const_iterator iter = directory_entry_.begin(); iter != directory_entry_.end(); iter++)
            {
                const DirectoryInfo& directory_info = iter->first;
                const DirectoryMetadata& directory_metadata = iter->second;
                if (directory_metadata.isValid()) // validity = true
                {
                    dirinfo_set.insert(directory_info);
                }
            }

            rwlock_.unlock_shared();
        }
        else // Unable to get read lock means with writes
        {
            is_being_written = true;
        }
        return is_being_written;
    }

    // (3) DirectoryTable

    DirectoryTable::DirectoryTable(const uint32_t& seed)
    {
        directory_hashtable_.clear();

        // Allocate randgen to choose target edge node from directory information
        directory_randgen_ptr_ = new std::mt19937_64(seed);
        assert(directory_randgen_ptr_ != NULL);
    }

    DirectoryTable::~DirectoryTable()
    {
        // Release randgen
        assert(directory_randgen_ptr_ != NULL);
        delete directory_randgen_ptr_;
        directory_randgen_ptr_ = NULL;
    }

    void DirectoryTable::lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info)
    {
        dirinfo_table_t::iterator directory_hashtable_iter = directory_hashtable_.find(key);
        dirinfo_set_t valid_directory_info_set;

        // Check if directory entry exists
        if (directory_hashtable_iter == directory_hashtable_.end()) // Directory entry does not exist
        {
            is_valid_directory_exist = false;
        }
        else // Directory entry exists
        {
            // Get all valid directory infos if without writes
            DirectoryEntry& directory_entry = directory_hashtable_iter->second;
            bool is_being_written = directory_entry.getValidDirinfoSet(valid_directory_info_set);

            // Check if key is being written
            if (is_being_written) // with writes
            {
                is_valid_directory_exist = false;
            }
            else // without writes
            {
                if (valid_directory_info_set.size() > 0) // At least one valid directory
                {
                    is_valid_directory_exist = true;
                }
                else // No valid directory
                {
                    is_valid_directory_exist = false;
                }
            } // End of if key is being written
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

    void DirectoryTable::update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        dirinfo_table_t::iterator directory_hashtable_iter = directory_hashtable_.find(key);
        if (is_admit) // Add a new directory info
        {
            // NOTE: validity flag must be true for each cache admission
            const bool validity = true;
            if (directory_hashtable_iter == directory_hashtable_.end()) // key does not exist
            {
                perkey_dirinfos_t tmp_dirinfos;
                tmp_dirinfos.insert(std::pair<DirectoryInfo, bool>(directory_info, validity));
                directory_hashtable_.insert(std::pair<Key, perkey_dirinfos_t>(key, tmp_dirinfos));
            }
            else // key already exists
            {
                perkey_dirinfos_t& tmp_dirinfos = directory_hashtable_iter->second;
                perkey_dirinfos_t::iterator tmp_dirinfos_iter = tmp_dirinfos.find(directory_info);
                if (tmp_dirinfos_iter != tmp_dirinfos.end()) // directory_info exists for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " already exists for key " << key.getKeystr() << " in update() with is_admit = true!";
                    Util::dumpWarnMsg(kClassName, oss.str());

                    tmp_dirinfos_iter->second = validity;
                }
                else // directory_info does not exist for key
                {
                    tmp_dirinfos.insert(std::pair<DirectoryInfo, bool>(directory_info, validity));
                }
            }
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            // NOTE: validity flag could be either true or false for each cache eviction
            if (directory_hashtable_iter != directory_hashtable_.end()) // key already exists
            {
                perkey_dirinfos_t& tmp_dirinfos = directory_hashtable_iter->second;
                if (tmp_dirinfos.find(directory_info) != tmp_dirinfos.end()) // directory_info already exists
                {
                    tmp_dirinfos.erase(directory_info);
                }
                else // directory_info does NOT exist
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
    }
}