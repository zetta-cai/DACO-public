/*
 * DirectoryCacher: ONLY cache valid remote directory for local uncached objects tracked by local uncached metadata (i.e, with large local admission benefits) (thread safe).
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

#include "concurrency/rwlock.h"
#include "cooperation/directory/directory_info.h"

namespace covered
{
    class DirectoryCacher
    {
    public:
        DirectoryCacher(const uint32_t& edge_idx);
        ~DirectoryCacher();

        bool getCachedDirinfo(const Key& key, DirectoryInfo& dirinfo) const; // Return if key has cached valid remote dirinfo
        void removeCachedDirinfoIfAny(const Key& key);
        void updateForNewCachedDirinfo(const Key&key, const DirectoryInfo& dirinfo); // Add or insert new cached dirinfo for the given key
    private:
        typedef std::unordered_map<Key, DirectoryInfo, KeyHasher> perkey_dirinfo_map_t;

        static const std::string kClassName;

        void checkPointers_() const;

        // Const shared variables
        std::string instance_name_;

        // For atomic access of non-const shared variables
        // NOTE: we do NOT use perkey_rwlock for fine-grained locking here, as # of cached directories is limited in DirectoryCacher and all other componenets in CoveredCacheManager cannot share PerkeyRwlock with Directorycacher
        mutable Rwlock* rwlock_for_directory_cacher_;

        // Non-const shared variables
        uint64_t size_bytes_; // Cache size usage of victim tracker
        perkey_dirinfo_map_t perkey_dirinfo_map_; // Valid reomte directory info cached for each local cached key tracked by local uncached metadata
    };
}

#endif