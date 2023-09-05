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

    uint32_t VictimCacheinfo::getVictimCacheinfoPayloadSize() const
    {
        uint32_t victim_cacheinfo_payload_size = 0;

        victim_cacheinfo_payload_size += key_.getKeyPayloadSize();
        victim_cacheinfo_payload_size += sizeof(ObjectSize);
        victim_cacheinfo_payload_size += sizeof(Popularity);
        victim_cacheinfo_payload_size += sizeof(Popularity);

        return victim_cacheinfo_payload_size;
    }

    uint32_t VictimCacheinfo::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&object_size_, sizeof(ObjectSize));
        size += sizeof(ObjectSize);
        msg_payload.deserialize(size, (const char*)&local_cached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        msg_payload.deserialize(size, (const char*)&redirected_cached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        return size - position;
    }

    uint32_t VictimCacheinfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&object_size_, sizeof(ObjectSize));
        size += sizeof(ObjectSize);
        msg_payload.serialize(size, (char *)&local_cached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        msg_payload.serialize(size, (char *)&redirected_cached_popularity_, sizeof(Popularity));
        size += sizeof(Popularity);
        return size - position;
    }

    uint64_t VictimCacheinfo::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += key_.getKeyLength();
        total_size += sizeof(ObjectSize);
        total_size += sizeof(Popularity);
        total_size += sizeof(Popularity);

        return total_size;
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