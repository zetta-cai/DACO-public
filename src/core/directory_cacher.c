#include "core/directory_cacher.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    // CachedDirectory

    const std::string CachedDirectory::kClassName("CachedDirectory");

    CachedDirectory::CachedDirectory() : dirinfo_(), prev_collect_popularity_(0.0) {}

    CachedDirectory::CachedDirectory(const DirectoryInfo& dirinfo, const Popularity& prev_collect_popularity)
    {
        dirinfo_ = dirinfo;
        prev_collect_popularity_ = prev_collect_popularity;
    }

    CachedDirectory::~CachedDirectory() {}

    DirectoryInfo CachedDirectory::getDirinfo() const
    {
        return dirinfo_;
    }

    Popularity CachedDirectory::getPrevCollectPopularity() const
    {
        return prev_collect_popularity_;
    }

    uint64_t CachedDirectory::getSizeForCapacity() const
    {
        uint64_t size_bytes = dirinfo_.getSizeForCapacity() + sizeof(Popularity);
        return size_bytes;
    }

    const CachedDirectory& CachedDirectory::operator=(const CachedDirectory& other)
    {
        dirinfo_ = other.dirinfo_;
        prev_collect_popularity_ = other.prev_collect_popularity_;

        return *this;
    }

    // Dump/load each of cached directories for cooperation snapshot

    void CachedDirectory::dumpCachedDirectory(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);
        
        // Dump the directory info
        dirinfo_.serialize(fs_ptr);

        // Dump prev collected popularity
        fs_ptr->write((const char*)&prev_collect_popularity_, sizeof(Popularity));

        return;
    }

    void CachedDirectory::loadCachedDirectory(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load the directory info
        dirinfo_.deserialize(fs_ptr);

        // Load prev collected popularity
        fs_ptr->read((char*)&prev_collect_popularity_, sizeof(Popularity));

        return;
    }

    // DirectoryCacher

    const std::string DirectoryCacher::kClassName("DirectoryCacher");

    DirectoryCacher::DirectoryCacher(const uint32_t& edge_idx, const double& popularity_collection_change_ratio) : popularity_collection_change_ratio_(popularity_collection_change_ratio)
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

    bool DirectoryCacher::getCachedDirectory(const Key& key, CachedDirectory& cached_directory) const
    {
        checkPointers_();
        
        // Acquire a read lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::getCachedDirectory()";
        rwlock_for_directory_cacher_->acquire_lock_shared(context_name);

        bool has_cached_directory = false;

        // Get cached directory if any
        perkey_dirinfo_map_t::const_iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end())
        {
            has_cached_directory = true;
            cached_directory = map_iter->second;
        }

        rwlock_for_directory_cacher_->unlock_shared(context_name);

        return has_cached_directory;
    }

    bool DirectoryCacher::checkPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const
    {
        checkPointers_();

        // Acquire a read lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::checkPopularityChange()";
        rwlock_for_directory_cacher_->acquire_lock_shared(context_name);

        bool has_cached_directory = false;

        // Get cached directory if any
        perkey_dirinfo_map_t::const_iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end())
        {
            has_cached_directory = true;
            cached_directory = map_iter->second;

            // Check if popularity change is large
            Popularity prev_collect_popularity = cached_directory.getPrevCollectPopularity();
            Popularity popularity_change = Util::popularityAbsMinus(local_uncached_popularity, prev_collect_popularity);
            assert(popularity_change >= 0);
            is_large_popularity_change = (popularity_change >= Util::popularityMultiply(popularity_collection_change_ratio_, prev_collect_popularity));
        }

        rwlock_for_directory_cacher_->unlock_shared(context_name);

        return has_cached_directory;
    }

    void DirectoryCacher::removeCachedDirectoryIfAny(const Key& key)
    {
        checkPointers_();
        
        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::removeCachedDirectoryIfAny()";
        rwlock_for_directory_cacher_->acquire_lock(context_name);

        // Remove cached directory if any
        perkey_dirinfo_map_t::iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end())
        {
            size_bytes_ = Util::uint64Minus(size_bytes_, map_iter->second.getSizeForCapacity());
            perkey_dirinfo_map_.erase(map_iter);
        }

        rwlock_for_directory_cacher_->unlock(context_name);

        return;
    }

    uint64_t DirectoryCacher::getSizeForCapacity() const
    {
        checkPointers_();

        // NOTE: NO need to acquire a read lock as approxiate cache size usage is enough

        return size_bytes_;
    }

    // Dump/load cached directories for cooperation snapshot

    void DirectoryCacher::dumpDirectoryCacher(std::fstream* fs_ptr) const
    {
        checkPointers_();
        assert(fs_ptr != NULL);

        // Dump size_bytes_
        fs_ptr->write((const char*)&size_bytes_, sizeof(uint64_t));

        // Dump per-key dirinfos
        // (1) per-key dirinfo cnt
        uint32_t perkey_dirinfo_cnt = perkey_dirinfo_map_.size();
        fs_ptr->write((const char*)&perkey_dirinfo_cnt, sizeof(uint32_t));
        // (2) per-key dirinfos
        for (perkey_dirinfo_map_t::const_iterator map_iter = perkey_dirinfo_map_.begin(); map_iter != perkey_dirinfo_map_.end(); ++map_iter)
        {
            // Dump the key
            const Key& tmp_key = map_iter->first;
            tmp_key.serialize(fs_ptr);

            // Dump the cached directory
            const CachedDirectory& tmp_cached_directory = map_iter->second;
            tmp_cached_directory.dumpCachedDirectory(fs_ptr);
        }

        return;
    }

    void DirectoryCacher::loadDirectoryCacher(std::fstream* fs_ptr)
    {
        checkPointers_();
        assert(fs_ptr != NULL);

        // Load size_bytes_
        fs_ptr->read((char*)&size_bytes_, sizeof(uint64_t));

        // Load per-key dirinfos
        // (1) per-key dirinfo cnt
        uint32_t perkey_dirinfo_cnt = 0;
        fs_ptr->read((char*)&perkey_dirinfo_cnt, sizeof(uint32_t));
        // (2) per-key dirinfos
        perkey_dirinfo_map_.clear();
        for (uint32_t i = 0; i < perkey_dirinfo_cnt; ++i)
        {
            // Load the key
            Key tmp_key;
            tmp_key.deserialize(fs_ptr);

            // Load the cached directory
            CachedDirectory tmp_cached_directory;
            tmp_cached_directory.loadCachedDirectory(fs_ptr);

            // Insert new cached directory
            perkey_dirinfo_map_.insert(std::pair(tmp_key, tmp_cached_directory));
        }

        return;
    }

    void DirectoryCacher::updateForNewCachedDirectory(const Key&key, const CachedDirectory& cached_directory)
    {
        checkPointers_();
        
        // Acquire a write lock to update victim dirinfo atomically
        std::string context_name = "DirectoryCacher::updateForNewCachedDirectory()";
        rwlock_for_directory_cacher_->acquire_lock(context_name);

        perkey_dirinfo_map_t::iterator map_iter = perkey_dirinfo_map_.find(key);
        if (map_iter != perkey_dirinfo_map_.end()) // If key already has a cached directory
        {
            map_iter->second = cached_directory; // Update with new cached directory
        }
        else
        {
            // Insert new cached directory
            size_bytes_ = Util::uint64Add(size_bytes_, cached_directory.getSizeForCapacity());
            perkey_dirinfo_map_.insert(std::pair(key, cached_directory));
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