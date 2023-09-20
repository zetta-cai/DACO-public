/*
 * DirectoryCacher: ONLY cache valid remote (i.e., neighbor beaconed) directory for local uncached objects tracked by local uncached metadata (i.e, with large approximate admission benefits) (thread safe).
 *
 * NOTE: NO need to introduce directory metadata cache max mem usage bytes, as we have limited the cache size capacity bytes of local uncached metadata and ONLY cache valid remote directory for such tracked local uncached objects.
 *
 * NOTE: we remove remote directory from DirectoryCacher if (i) remote directory becomes invalid or missed; (ii) key becomes local cached; (iii) key is still local uncached yet becomes untracked by local uncached metadata.
 * 
 * By Siyuan Sheng (2023.09.12).
 */

#ifndef DIRECTORY_CACHER_H
#define DIRECTORY_CACHER_H

#include <string>
#include <unordered_map>

#include "common/covered_common_header.h"
#include "concurrency/rwlock.h"
#include "cooperation/directory/directory_info.h"

namespace covered
{
    class CachedDirectory
    {
    public:
        CachedDirectory();
        CachedDirectory(const DirectoryInfo& dirinfo, const Popularity& prev_collect_popularity);
        ~CachedDirectory();

        DirectoryInfo getDirinfo() const;
        Popularity getPrevCollectPopularity() const;

        uint64_t getSizeForCapacity() const;

        const CachedDirectory& operator=(const CachedDirectory& other);
    private:
        static const std::string kClassName;

        DirectoryInfo dirinfo_; // Valid remote dirinfo
        Popularity prev_collect_popularity_; // Previously-collected popularity
    };

    class DirectoryCacher
    {
    public:
        DirectoryCacher(const uint32_t& edge_idx, const double& popularity_collection_change_ratio);
        ~DirectoryCacher();

        bool getCachedDirectory(const Key& key, CachedDirectory& cached_directory) const; // Return if key has cached valid remote dirinfo
        bool checkPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const; // Return if key has cached valid remote dirinfo
        void removeCachedDirectoryIfAny(const Key& key);
        void updateForNewCachedDirectory(const Key&key, const CachedDirectory& cached_directory); // Add or insert new cached directory for the given key
    private:
        typedef std::unordered_map<Key, CachedDirectory, KeyHasher> perkey_dirinfo_map_t;

        static const std::string kClassName;

        void checkPointers_() const;

        // Const shared variables
        std::string instance_name_;
        const double popularity_collection_change_ratio_; // Come from CLI

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as # of cached directories is limited in DirectoryCacher and all other componenets in CoveredCacheManager cannot share PerkeyRwlock with Directorycacher
        mutable Rwlock* rwlock_for_directory_cacher_;

        // Non-const shared variables
        uint64_t size_bytes_; // Cache size usage of victim tracker
        perkey_dirinfo_map_t perkey_dirinfo_map_; // Valid reomte directory info cached for each local cached key tracked by local uncached metadata
    };
}

#endif