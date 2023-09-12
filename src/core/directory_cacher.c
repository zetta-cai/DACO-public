#include "core/directory_cacher.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DirectoryCacher::kClassName("DirectoryCacher");

    DirectoryCacher::DirectoryCacher(const uint32_t& edge_idx)
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_directory_cacher_";
        rwlock_for_directory_cacher_ = new Rwlock(oss.str());
        assert(rwlock_for_directory_cacher_ != NULL);

        size_bytes_ = 0;
        perkey_dirinfo_map_.clear();
    }

    DirectoryCacher::~DirectoryCacher()
    {
        assert(rwlock_for_directory_cacher_ != NULL);
        delete rwlock_for_directory_cacher_;
        rwlock_for_directory_cacher_ = NULL;
    }

    bool DirectoryCacher::getCachedDirinfo(const Key& key, DirectoryInfo& dirinfo) const
    {
        checkPointers_();
        
        // Acquire a read lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::getCachedDirinfo()";
        rwlock_for_directory_cacher_->acquire_lock_shared(context_name);

        bool has_cached_dirinfo = false;

        // Get cached dirinfo if any
        perkey_dirinfo_map_t::const_iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end())
        {
            has_cached_dirinfo = true;
            dirinfo = map_iter->second;
        }

        rwlock_for_directory_cacher_->unlock_shared(context_name);

        return has_cached_dirinfo;
    }

    void DirectoryCacher::removeCachedDirinfoIfAny(const Key& key)
    {
        checkPointers_();
        
        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::removeCachedDirinfoIfAny()";
        rwlock_for_directory_cacher_->acquire_lock(context_name);

        // Remove cached dirinfo if any
        perkey_dirinfo_map_t::iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end())
        {
            size_bytes_ = Util::uint64Minus(size_bytes_, map_iter->second.getSizeForCapacity());
            perkey_dirinfo_map_.erase(map_iter);
        }

        rwlock_for_directory_cacher_->unlock(context_name);

        return;
    }

    void DirectoryCacher::updateForNewCachedDirinfo(const Key&key, const DirectoryInfo& dirinfo)
    {
        checkPointers_();
        
        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::updateForNewCachedDirinfo()";
        rwlock_for_directory_cacher_->acquire_lock(context_name);

        perkey_dirinfo_map_t::iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end()) // If key already has a cached dirinfo
        {
            map_iter->second = dirinfo; // Update with new cached dirinfo
        }
        else
        {
            // Insert new cached dirinfo
            size_bytes_ = Util::uint64Add(size_bytes_, dirinfo.getSizeForCapacity());
            perkey_dirinfo_map_.insert(std::pair(key, dirinfo));
        }

        rwlock_for_directory_cacher_->unlock(context_name);

        return;
    }

    void DirectoryCacher::checkPointers_() const
    {
        assert(rwlock_for_directory_cacher_ != NULL);
        return;
    }
}