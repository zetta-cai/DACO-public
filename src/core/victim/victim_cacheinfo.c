#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    const std::string VictimCacheinfo::kClassName = "VictimCacheinfo";

    VictimCacheinfo::VictimCacheinfo() : key_(), object_size_(0), local_cached_popularity_(0.0), redirected_cached_popularity_(0.0)
    {
    }

    VictimCacheinfo::VictimCacheinfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity)
    {
        key_ = key;
        object_size_ = object_size;
        local_cached_popularity_ = local_cached_popularity;
        redirected_cached_popularity_ = redirected_cached_popularity;
    }

    VictimCacheinfo::~VictimCacheinfo() {}

    const Key VictimCacheinfo::getKey() const
    {
        return key_;
    }

    const ObjectSize VictimCacheinfo::getObjectSize() const
    {
        return object_size_;
    }

    const Popularity VictimCacheinfo::getLocalCachedPopularity() const
    {
        return local_cached_popularity_;
    }

    const Popularity VictimCacheinfo::getRedirectedCachedPopularity() const
    {
        return redirected_cached_popularity_;
    }

    const VictimCacheinfo& VictimCacheinfo::operator=(const VictimCacheinfo& other)
    {
        key_ = other.key_;
        object_size_ = other.object_size_;
        local_cached_popularity_ = other.local_cached_popularity_;
        redirected_cached_popularity_ = other.redirected_cached_popularity_;
        
        return *this;
    }
}