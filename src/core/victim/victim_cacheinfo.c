#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    const std::string VictimCacheinfo::kClassName = "VictimCacheinfo";

    const uint8_t VictimCacheinfo::INVALID_BITMAP = 0b11111111;
    const uint8_t VictimCacheinfo::COMPLETE_BITMAP = 0b00000000;
    const uint8_t VictimCacheinfo::STALE_BITMAP = 0b00001111;
    const uint8_t VictimCacheinfo::OBJECT_SIZE_DEDUP_MASK = 0b00000011;
    const uint8_t VictimCacheinfo::LOCAL_CACHED_POPULARITY_DEDUP_MASK = 0b00000101;
    const uint8_t VictimCacheinfo::REDIRECTED_CACHED_POPULARITY_DEDUP_MASK = 0b00001001;

    VictimCacheinfo::VictimCacheinfo() : dedup_bitmap_(INVALID_BITMAP), key_(), object_size_(0), local_cached_popularity_(0.0), redirected_cached_popularity_(0.0)
    {
    }

    VictimCacheinfo::VictimCacheinfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity)
    {
        dedup_bitmap_ = COMPLETE_BITMAP;
        key_ = key;
        object_size_ = object_size;
        local_cached_popularity_ = local_cached_popularity;
        redirected_cached_popularity_ = redirected_cached_popularity;
    }

    VictimCacheinfo::~VictimCacheinfo() {}

    bool VictimCacheinfo::isComplete() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return (dedup_bitmap_ == COMPLETE_BITMAP);
    }

    bool VictimCacheinfo::isStale() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return (dedup_bitmap_ == STALE_BITMAP);
    }

    bool VictimCacheinfo::isDeduped() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return (dedup_bitmap_ != COMPLETE_BITMAP && dedup_bitmap_ != STALE_BITMAP);
    }

    const Key VictimCacheinfo::getKey() const
    {
        return key_;
    }

    bool VictimCacheinfo::getObjectSize(ObjectSize& object_size) const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        bool with_complete_object_size = ((dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) != OBJECT_SIZE_DEDUP_MASK);
        if (with_complete_object_size) // NOT deduped
        {
            object_size = object_size_;
        }
        return with_complete_object_size;
    }

    bool VictimCacheinfo::getLocalCachedPopularity(Popularity& local_cached_popularity) const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_cached_popularity = ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) != LOCAL_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_local_cached_popularity) // NOT deduped
        {
            local_cached_popularity = local_cached_popularity_;
        }
        return with_complete_local_cached_popularity;
    }

    bool VictimCacheinfo::getRedirectedCachedPopularity(Popularity& redirected_cached_popularity) const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        bool with_complete_redirected_cached_popularity = ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) != REDIRECTED_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_redirected_cached_popularity) // NOT deduped
        {
            redirected_cached_popularity = redirected_cached_popularity_;
        }
        return with_complete_redirected_cached_popularity;
    }

    uint32_t VictimCacheinfo::getVictimCacheinfoPayloadSize() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        uint32_t victim_cacheinfo_payload_size = 0;

        victim_cacheinfo_payload_size += sizeof(uint8_t); // dedup bitmap
        victim_cacheinfo_payload_size += key_.getKeyPayloadSize(); // key
        bool with_complete_object_size = ((dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) != OBJECT_SIZE_DEDUP_MASK);
        if (with_complete_object_size)
        {
            victim_cacheinfo_payload_size += sizeof(ObjectSize); // object size
        }
        bool with_complete_local_cached_popularity = ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) != LOCAL_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_local_cached_popularity)
        {
            victim_cacheinfo_payload_size += sizeof(Popularity); // local cached popularity
        }
        bool with_complete_redirected_cached_popularity = ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) != REDIRECTED_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_redirected_cached_popularity)
        {
            victim_cacheinfo_payload_size += sizeof(Popularity); // redirected cached popularity
        }

        return victim_cacheinfo_payload_size;
    }

    uint32_t VictimCacheinfo::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&dedup_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        bool with_complete_object_size = ((dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) != OBJECT_SIZE_DEDUP_MASK);
        if (with_complete_object_size)
        {
            msg_payload.deserialize(size, (const char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);
        }
        bool with_complete_local_cached_popularity = ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) != LOCAL_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_local_cached_popularity)
        {
            msg_payload.deserialize(size, (const char*)&local_cached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
        }
        bool with_complete_redirected_cached_popularity = ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) != REDIRECTED_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_redirected_cached_popularity)
        {
            msg_payload.deserialize(size, (const char*)&redirected_cached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
        }
        return size - position;
    }

    uint32_t VictimCacheinfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        uint32_t size = position;
        msg_payload.serialize(size, (char*)&dedup_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        bool with_complete_object_size = ((dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) != OBJECT_SIZE_DEDUP_MASK);
        if (with_complete_object_size)
        {
            msg_payload.serialize(size, (char*)&object_size_, sizeof(ObjectSize));
            size += sizeof(ObjectSize);
        }
        bool with_complete_local_cached_popularity = ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) != LOCAL_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_local_cached_popularity)
        {
            msg_payload.serialize(size, (char*)&local_cached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
        }
        bool with_complete_redirected_cached_popularity = ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) != REDIRECTED_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_redirected_cached_popularity)
        {
            msg_payload.serialize(size, (char*)&redirected_cached_popularity_, sizeof(Popularity));
            size += sizeof(Popularity);
        }
        return size - position;
    }

    uint64_t VictimCacheinfo::getSizeForCapacity() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);
        
        // NOTE: ONLY complete victim cacheinfo will consume metadata space for per-edge synced victims and per-edge prev victim syncset in victim tracker
        assert(isComplete() == true);
        const bool with_complete_object_size = true;
        const bool with_complete_local_cached_popularity = true;
        const bool with_complete_redirected_cached_popularity = true;

        uint64_t total_size = 0;

        total_size += sizeof(uint8_t); // dedup bitmap
        total_size += key_.getKeyLength(); // key
        if (with_complete_object_size)
        {
            total_size += sizeof(ObjectSize);
        }
        if (with_complete_local_cached_popularity)
        {
            total_size += sizeof(Popularity);
        }
        if (with_complete_redirected_cached_popularity)
        {
            total_size += sizeof(Popularity);
        }

        return total_size;
    }

    const VictimCacheinfo& VictimCacheinfo::operator=(const VictimCacheinfo& other)
    {
        dedup_bitmap_ = other.dedup_bitmap_;
        key_ = other.key_;
        object_size_ = other.object_size_;
        local_cached_popularity_ = other.local_cached_popularity_;
        redirected_cached_popularity_ = other.redirected_cached_popularity_;
        
        return *this;
    }
}