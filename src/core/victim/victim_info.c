#include "core/victim/victim_info.h"

namespace covered
{
    const std::string VictimInfo::kClassName = "VictimInfo";

    VictimInfo::VictimInfo() : key_(), object_size_(0), local_cached_popularity_(0.0), redirected_cached_popularity_(0.0)
    {
    }

    VictimInfo::VictimInfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity)
    {
        key_ = key;
        object_size_ = object_size;
        local_cached_popularity_ = local_cached_popularity;
        redirected_cached_popularity_ = redirected_cached_popularity;
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

    const VictimInfo& VictimInfo::operator=(const VictimInfo& other)
    {
        key_ = other.key_;
        object_size_ = other.object_size_;
        local_cached_popularity_ = other.local_cached_popularity_;
        redirected_cached_popularity_ = other.redirected_cached_popularity_;
        
        return *this;
    }
}