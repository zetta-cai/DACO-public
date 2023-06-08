#include "cooperation/directory_table.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryTable::kClassName("DirectoryTable");

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

    void DirectoryTable::lookup(const Key& key, bool& is_directory_exist, DirectoryInfo& directory_info) const
    {
        // Check directory information cooperation_wrapper_ptr_
        dirinfo_set_t directory_info_set;
        dirinfo_table_t::const_iterator iter = directory_hashtable_.find(key);
        if (iter != directory_hashtable_.end())
        {
            is_directory_exist = true;
            directory_info_set = iter->second;
        }
        else
        {
            is_directory_exist = false;
        }

        // Get the target edge index
        if (is_directory_exist)
        {
            // Randomly select an edge node as the target edge node
            std::uniform_int_distribution<uint32_t> uniform_dist(0, directory_info_set.size() - 1); // Range from 0 to (# of directory info - 1)
            uint32_t random_number = uniform_dist(*directory_randgen_ptr_);
            assert(random_number < directory_info_set.size());
            uint32_t i = 0;
            for (dirinfo_set_t::const_iterator iter = directory_info_set.begin(); iter != directory_info_set.end(); iter++)
            {
                if (i == random_number)
                {
                    directory_info = *iter;
                    break;
                }
                i++;
            }
        }
    }

    void DirectoryTable::update(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        dirinfo_table_t::iterator iter = directory_hashtable_.find(key);
        if (is_admit) // Add a new directory info
        {
            if (iter == directory_hashtable_.end()) // key does not exist
            {
                dirinfo_set_t tmp_directory_info_set;
                tmp_directory_info_set.insert(directory_info);
                directory_hashtable_.insert(std::pair<Key, dirinfo_set_t>(key, tmp_directory_info_set));
            }
            else // key already exists
            {
                dirinfo_set_t& tmp_directory_info_set = iter->second;
                if (tmp_directory_info_set.find(directory_info) != tmp_directory_info_set.end()) // directory_info exists for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << directory_info.getTargetEdgeIdx() << " already exists for key " << key.getKeystr() << " in update() with is_admit = true!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
                else // directory_info does not exist for key
                {
                    tmp_directory_info_set.insert(directory_info);
                }
            }
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            std::ostringstream oss;
            oss << "target edge index " << directory_info.getTargetEdgeIdx() << " does NOT exist for key " << key.getKeystr() << " in update() with is_admit = false!";
            if (iter != directory_hashtable_.end()) // key already exists
            {
                dirinfo_set_t& tmp_directory_info_set = iter->second;
                if (tmp_directory_info_set.find(directory_info) != tmp_directory_info_set.end()) // directory_info already exists
                {
                    tmp_directory_info_set.erase(directory_info);
                }
                else // directory_info does NOT exist
                {
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
            }
            else // key does NOT exist
            {
                Util::dumpWarnMsg(kClassName, oss.str());
            }
        } // ENd of (is_admit == false)
    }
}