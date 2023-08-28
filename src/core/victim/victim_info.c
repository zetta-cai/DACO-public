#include "core/victim/victim_info.h"

namespace covered
{
    const std::string VictimInfo::kClassName = "VictimInfo";

    VictimInfo::VictimInfo() : key_(), object_size_(0), local_cached_popularity_(0.0), redirected_cached_popularity_(0.0), dirinfo_()
    {
    }

    VictimInfo::VictimInfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const DirectoryInfo& dirinfo)
    {
        key_ = key;
        object_size_ = object_size;
        local_cached_popularity_ = local_cached_popularity;
        redirected_cached_popularity_ = redirected_cached_popularity;
        dirinfo_ = dirinfo;
    }

    VictimInfo::~VictimInfo() {}

    const Key& VictimInfo::getKey() const
    {
        return key_;
    }

    const ObjectSize& VictimInfo::getObjectSize() const
    {
        return object_size_;
    }

    const Popularity& VictimInfo::getLocalCachedPopularity() const
    {
        return local_cached_popularity_;
    }

    const Popularity& VictimInfo::getRedirectedCachedPopularity() const
    {
        return redirected_cached_popularity_;
    }

    const DirectoryInfo& VictimInfo::getDirinfo() const
    {
        return dirinfo_;
    }

    const VictimInfo& VictimInfo::operator=(const VictimInfo& other)
    {
        key_ = other.key_;
        local_cached_popularity_ = other.local_cached_popularity_;
        redirected_cached_popularity_ = other.redirected_cached_popularity_;
        dirinfo_ = other.dirinfo_;
        
        return *this;
    }
}