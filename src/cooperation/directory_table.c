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

    DirectoryMetadata& DirectoryMetadata::operator=(const DirectoryMetadata& other)
    {
        is_valid_ = other.is_valid_;
        return *this;
    }

    // (2) DirectoryEntry

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
            if (directory_metadata.isValid()) // validity = true
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

    // (3) DirectoryTable

    DirectoryTable::DirectoryTable(const uint32_t& seed, const uint32_t& edge_idx)
    {
        // Allocate randgen to choose target edge node from directory information
        directory_randgen_ptr_ = new std::mt19937_64(seed);
        assert(directory_randgen_ptr_ != NULL);

        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_dirtable_ptr_";
        rwlock_for_dirtable_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_dirtable_ptr_ != NULL);

        directory_hashtable_.clear();
    }

    DirectoryTable::~DirectoryTable()
    {
        // Release randgen
        assert(directory_randgen_ptr_ != NULL);
        delete directory_randgen_ptr_;
        directory_randgen_ptr_ = NULL;

        assert(rwlock_for_dirtable_ptr_ != NULL);
        delete rwlock_for_dirtable_ptr_;
        rwlock_for_dirtable_ptr_ = NULL;
    }

    void DirectoryTable::lookup(const Key& key, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // NOTE: as writer(s) can update DirectoryTable very quickly, it is okay to polling rwlock_ here
        assert(rwlock_for_dirtable_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_dirtable_ptr_->try_lock_shared("lookup()"))
            {
                break;
            }
        }

        dirinfo_table_t::const_iterator directory_hashtable_iter = directory_hashtable_.find(key);
        dirinfo_set_t valid_directory_info_set;

        // Check if key exists
        if (directory_hashtable_iter == directory_hashtable_.end()) // key does not exist
        {
            is_valid_directory_exist = false;
        }
        else // key exists
        {
            // Get all valid directory infos if any
            const DirectoryEntry& directory_entry = directory_hashtable_iter->second;
            directory_entry.getValidDirinfoSet(valid_directory_info_set);

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

        rwlock_for_dirtable_ptr_->unlock_shared();
        return;
    }

    void DirectoryTable::update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const DirectoryMetadata& directory_metadata)
    {
        // NOTE: as writer(s) can update DirectoryTable very quickly, it is okay to polling rwlock_ here
        assert(rwlock_for_dirtable_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_dirtable_ptr_->try_lock("update()"))
            {
                break;
            }
        }

        dirinfo_table_t::iterator directory_hashtable_iter = directory_hashtable_.find(key);
        if (is_admit) // Add a new directory info
        {
            // Check if key exists
            if (directory_hashtable_iter == directory_hashtable_.end()) // key does not exist
            {
                assert(directory_metadata.isValid()); // key must NOT being written
                DirectoryEntry directory_entry;
                bool is_directory_already_exists = directory_entry.addDirinfo(directory_info, directory_metadata);
                assert(is_directory_already_exists == false);
                directory_hashtable_.insert(std::pair<Key, DirectoryEntry>(key, directory_entry));
            }
            else // key already exists
            {
                DirectoryEntry& directory_entry = directory_hashtable_iter->second;
                bool is_directory_already_exists = directory_entry.addDirinfo(directory_info, directory_metadata);
                if (is_directory_already_exists) // directory_info already exists for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " already exists for key " << key.getKeystr() << " in update() with is_admit = true!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
            }
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            // Check if key exists
            if (directory_hashtable_iter != directory_hashtable_.end()) // key already exists
            {
                DirectoryEntry& directory_entry = directory_hashtable_iter->second;
                bool is_directory_already_exists = directory_entry.removeDirinfo(directory_info);
                if (!is_directory_already_exists) // directory_info does NOT exist
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

        rwlock_for_dirtable_ptr_->unlock();
        return;
    }
}