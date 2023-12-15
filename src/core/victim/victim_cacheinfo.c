#include "core/victim/victim_cacheinfo.h"

#include "common/util.h"

namespace covered
{
    const std::string VictimCacheinfo::kClassName = "VictimCacheinfo";

    const uint8_t VictimCacheinfo::INVALID_BITMAP = 0b11111111;
    const uint8_t VictimCacheinfo::COMPLETE_BITMAP = 0b00000000;
    const uint8_t VictimCacheinfo::DEDUP_MASK = 0b00000001;
    // NOTE: use STALE_BITMAP (i.e., all five lowest bits are 1) to indicate that the stale victim cacheinfo of key_ needs to be removed (i.e., key_ is NOT a local synced victim)
    // NOTE: although we do NOT need to sync victim cache info if all three fields are deduped, we do NOT use 0x00001111 as STALE_BITMAP to avoid confusion
    const uint8_t VictimCacheinfo::STALE_BITMAP = 0b00111111;
    const uint8_t VictimCacheinfo::OBJECT_SIZE_DEDUP_MASK = 0b00000010 | DEDUP_MASK;
    const uint8_t VictimCacheinfo::LOCAL_CACHED_POPULARITY_DEDUP_MASK = 0b00000100 | DEDUP_MASK;
    const uint8_t VictimCacheinfo::REDIRECTED_CACHED_POPULARITY_DEDUP_MASK = 0b00001000 | DEDUP_MASK;
    const uint8_t VictimCacheinfo::LOCAL_REWARD_DEDUP_MASK = 0b00010000 | DEDUP_MASK;

    VictimCacheinfo VictimCacheinfo::dedup(const VictimCacheinfo& current_victim_cacheinfo, const VictimCacheinfo& prev_victim_cacheinfo)
    {
        // Perform deduplication based on two complete victim cacheinfos
        assert(current_victim_cacheinfo.isComplete() == true);
        assert(prev_victim_cacheinfo.isComplete() == true);

        VictimCacheinfo deduped_victim_cacheinfo = current_victim_cacheinfo;

        // (1) Perform dedup on object size
        ObjectSize current_object_size = current_victim_cacheinfo.object_size_;
        ObjectSize prev_object_size = prev_victim_cacheinfo.object_size_;
        if (current_object_size == prev_object_size)
        {
            deduped_victim_cacheinfo.dedupObjectSize();
        }

        // (2) Perform dedup on local cached popularity
        Popularity current_local_cached_popularity = current_victim_cacheinfo.local_cached_popularity_;
        Popularity prev_local_cached_popularity = prev_victim_cacheinfo.local_cached_popularity_;
        if (Util::isApproxEqual(current_local_cached_popularity, prev_local_cached_popularity))
        {
            deduped_victim_cacheinfo.dedupLocalCachedPopularity();
        }

        // (3) Perform dedup on redirected cached popularity
        Popularity current_redirected_cached_popularity = current_victim_cacheinfo.redirected_cached_popularity_;
        Popularity prev_redirected_cached_popularity = prev_victim_cacheinfo.redirected_cached_popularity_;
        if (Util::isApproxEqual(current_redirected_cached_popularity, prev_redirected_cached_popularity))
        {
            deduped_victim_cacheinfo.dedupRedirectedCachedPopularity();
        }

        // (4) Perform dedup on local reward
        Reward current_local_reward = current_victim_cacheinfo.local_reward_;
        Reward prev_local_reward = prev_victim_cacheinfo.local_reward_;
        if (Util::isApproxEqual(current_local_reward, prev_local_reward))
        {
            deduped_victim_cacheinfo.dedupLocalReward();
        }

        // (5) Get final victim cacheinfo
        const uint32_t deduped_victim_cacheinfo_payload_size = deduped_victim_cacheinfo.getVictimCacheinfoPayloadSize();
        const uint32_t current_victim_cacheinfo_payload_size = current_victim_cacheinfo.getVictimCacheinfoPayloadSize();
        if (deduped_victim_cacheinfo_payload_size < current_victim_cacheinfo_payload_size)
        {
            #ifdef DEBUG_VICTIM_CACHEINFO
            Util::dumpVariablesForDebug(kClassName, 5, "use deduped victim cacheinfo!", "deduped_victim_cacheinfo_payload_size:", std::to_string(deduped_victim_cacheinfo_payload_size).c_str(), "current_victim_cacheinfo_payload_size:", std::to_string(current_victim_cacheinfo_payload_size).c_str());
            #endif

            return deduped_victim_cacheinfo;
        }
        else
        {
            #ifdef DEBUG_VICTIM_CACHEINFO
                Util::dumpVariablesForDebug(kClassName, 5, "use complete victim cacheinfo!", "deduped_victim_cacheinfo_payload_size:", std::to_string(deduped_victim_cacheinfo_payload_size).c_str(), "current_victim_cacheinfo_payload_size:", std::to_string(current_victim_cacheinfo_payload_size).c_str());
            #endif

            return current_victim_cacheinfo;
        }
    }

    VictimCacheinfo VictimCacheinfo::recover(const VictimCacheinfo& compressed_victim_cacheinfo, const VictimCacheinfo& existing_victim_cacheinfo)
    {
        // Recover current complete victim cacheinfo based on received compressed yet non-fully-deduped victim cacheinfo and prev complete victim cacheinfo
        assert(compressed_victim_cacheinfo.isDeduped());
        assert(!compressed_victim_cacheinfo.isFullyDeduped());
        assert(existing_victim_cacheinfo.isComplete());

        VictimCacheinfo complete_victim_cacheinfo = existing_victim_cacheinfo;

        // (1) Recover object size
        ObjectSize current_object_size = 0;
        bool with_complete_object_size = compressed_victim_cacheinfo.getObjectSize(current_object_size);
        if (with_complete_object_size)
        {
            complete_victim_cacheinfo.object_size_ = current_object_size;
        }

        // (2) Recover local cached popularity
        Popularity current_local_cached_popularity = 0.0;
        bool with_complete_local_cached_popularity = compressed_victim_cacheinfo.getLocalCachedPopularity(current_local_cached_popularity);
        if (with_complete_local_cached_popularity)
        {
            complete_victim_cacheinfo.local_cached_popularity_ = current_local_cached_popularity;
        }

        // (3) Recover redirected cached popularity
        Popularity current_redirected_cached_popularity = 0.0;
        bool with_complete_redirected_cached_popularity = compressed_victim_cacheinfo.getRedirectedCachedPopularity(current_redirected_cached_popularity);
        if (with_complete_redirected_cached_popularity)
        {
            complete_victim_cacheinfo.redirected_cached_popularity_ = current_redirected_cached_popularity;
        }

        // (4) Recover local reward
        Reward current_local_reward = 0.0;
        bool with_complete_local_reward = compressed_victim_cacheinfo.getLocalReward(current_local_reward);
        if (with_complete_local_reward)
        {
            complete_victim_cacheinfo.local_reward_ = current_local_reward;
        }

        assert(complete_victim_cacheinfo.isComplete());
        return complete_victim_cacheinfo;
    }

    void VictimCacheinfo::sortByLocalRewards(std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        // Sort victim cacheinfos by local rewards (ascending order)
        victim_cacheinfos.sort(VictimCacheinfoCompare());
        return;
    }

    // Find victim cacheinfo for the given key

    std::list<VictimCacheinfo>::iterator VictimCacheinfo::findVictimCacheinfoForKey(const Key& key, std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        std::list<VictimCacheinfo>::iterator iter = victim_cacheinfos.begin();
        for (; iter != victim_cacheinfos.end(); iter++)
        {
            if (iter->getKey() == key)
            {
                break;
            }
        }
        return iter;
    }

    std::list<VictimCacheinfo>::const_iterator VictimCacheinfo::findVictimCacheinfoForKey(const Key& key, const std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        std::list<VictimCacheinfo>::const_iterator const_iter = victim_cacheinfos.begin();
        for (; const_iter != victim_cacheinfos.end(); const_iter++)
        {
            if (const_iter->getKey() == key)
            {
                break;
            }
        }
        return const_iter;
    }

    VictimCacheinfo::VictimCacheinfo() : dedup_bitmap_(INVALID_BITMAP), key_(), object_size_(0), local_cached_popularity_(0.0), redirected_cached_popularity_(0.0), local_reward_(0.0)
    {
    }

    VictimCacheinfo::VictimCacheinfo(const Key& key, const ObjectSize& object_size, const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const Reward& local_reward)
    {
        dedup_bitmap_ = COMPLETE_BITMAP;
        key_ = key;
        object_size_ = object_size;
        local_cached_popularity_ = local_cached_popularity;
        redirected_cached_popularity_ = redirected_cached_popularity;
        local_reward_ = local_reward;
    }

    VictimCacheinfo::~VictimCacheinfo() {}

    bool VictimCacheinfo::isInvalid() const
    {
        return (dedup_bitmap_ == INVALID_BITMAP);
    }

    bool VictimCacheinfo::isComplete() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return (dedup_bitmap_ != STALE_BITMAP) && ((dedup_bitmap_ & DEDUP_MASK) == (COMPLETE_BITMAP & DEDUP_MASK));
    }

    bool VictimCacheinfo::isStale() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return dedup_bitmap_ == STALE_BITMAP;
    }

    bool VictimCacheinfo::isDeduped() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        return (dedup_bitmap_ != STALE_BITMAP) && ((dedup_bitmap_ & DEDUP_MASK) == DEDUP_MASK);
    }

    bool VictimCacheinfo::isFullyDeduped() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        // NOTE: STALE_BITMAP can also cover all dedup masks
        return ((dedup_bitmap_ != STALE_BITMAP) && (dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) == OBJECT_SIZE_DEDUP_MASK) && ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) == LOCAL_CACHED_POPULARITY_DEDUP_MASK) && ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) == REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) && ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) == LOCAL_REWARD_DEDUP_MASK);
    }

    // For complete victim cacheinfo

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

    bool VictimCacheinfo::getLocalReward(Reward& local_reward) const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_reward = ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) != LOCAL_REWARD_DEDUP_MASK);
        if (with_complete_local_reward) // NOT deduped
        {
            local_reward = local_reward_;
        }
        return with_complete_local_reward;
    }

    // For compressed victim cacheinfo

    void VictimCacheinfo::markStale()
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        dedup_bitmap_ = STALE_BITMAP;
        return;
    }
    
    void VictimCacheinfo::dedupObjectSize()
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        dedup_bitmap_ |= OBJECT_SIZE_DEDUP_MASK;
        object_size_ = 0;
        return;
    }

    void VictimCacheinfo::dedupLocalCachedPopularity()
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        dedup_bitmap_ |= LOCAL_CACHED_POPULARITY_DEDUP_MASK;
        local_cached_popularity_ = 0.0;
        return;
    }

    void VictimCacheinfo::dedupRedirectedCachedPopularity()
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        dedup_bitmap_ |= REDIRECTED_CACHED_POPULARITY_DEDUP_MASK;
        redirected_cached_popularity_ = 0.0;
        return;
    }

    void VictimCacheinfo::dedupLocalReward()
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);

        dedup_bitmap_ |= LOCAL_REWARD_DEDUP_MASK;
        local_reward_ = 0.0;
        return;
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
        bool with_complete_local_reward = ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) != LOCAL_REWARD_DEDUP_MASK);
        if (with_complete_local_reward)
        {
            victim_cacheinfo_payload_size += sizeof(Reward); // local reward
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
        bool with_complete_local_reward = ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) != LOCAL_REWARD_DEDUP_MASK);
        if (with_complete_local_reward)
        {
            msg_payload.deserialize(size, (const char*)&local_reward_, sizeof(Reward));
            size += sizeof(Reward);
        }

        return size - position;
    }

    uint32_t VictimCacheinfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&dedup_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        assert(dedup_bitmap_ != INVALID_BITMAP);
        
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
        bool with_complete_local_reward = ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) != LOCAL_REWARD_DEDUP_MASK);
        if (with_complete_local_reward)
        {
            msg_payload.serialize(size, (char*)&local_reward_, sizeof(Reward));
            size += sizeof(Reward);
        }

        return size - position;
    }

    uint64_t VictimCacheinfo::getSizeForCapacity() const
    {
        assert(dedup_bitmap_ != INVALID_BITMAP);
        
        // (OBSOLETE due to pregen_compressed_victim_syncset_ptr_ in VictimSyncMonitor) NOTE: ONLY complete victim cacheinfo will consume metadata space for per-edge synced victims and per-edge prev victim syncset in victim tracker
        // assert(isComplete() == true);
        // const bool with_complete_object_size = true;
        // const bool with_complete_local_cached_popularity = true;
        // const bool with_complete_redirected_cached_popularity = true;
        // const bool with_complete_local_reward = true;

        uint64_t total_size = 0;

        total_size += sizeof(uint8_t); // dedup bitmap
        total_size += key_.getKeyLength(); // key

        bool with_complete_object_size = ((dedup_bitmap_ & OBJECT_SIZE_DEDUP_MASK) != OBJECT_SIZE_DEDUP_MASK);
        if (with_complete_object_size)
        {
            total_size += sizeof(ObjectSize);
        }

        bool with_complete_local_cached_popularity = ((dedup_bitmap_ & LOCAL_CACHED_POPULARITY_DEDUP_MASK) != LOCAL_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_local_cached_popularity)
        {
            total_size += sizeof(Popularity);
        }

        bool with_complete_redirected_cached_popularity = ((dedup_bitmap_ & REDIRECTED_CACHED_POPULARITY_DEDUP_MASK) != REDIRECTED_CACHED_POPULARITY_DEDUP_MASK);
        if (with_complete_redirected_cached_popularity)
        {
            total_size += sizeof(Popularity);
        }

        bool with_complete_local_reward = ((dedup_bitmap_ & LOCAL_REWARD_DEDUP_MASK) != LOCAL_REWARD_DEDUP_MASK);
        if (with_complete_local_reward)
        {
            total_size += sizeof(Reward);
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
        local_reward_ = other.local_reward_;
        
        return *this;
    }

    bool VictimCacheinfo::operator<(const VictimCacheinfo& other) const
    {
        // NOTE: this is used for std::list::sort() in VictimSyncset::recover()
        return (local_reward_ < other.local_reward_);
    }

    bool VictimCacheinfoCompare::operator()(const VictimCacheinfo& lhs, const VictimCacheinfo& rhs) const
    {
        return lhs < rhs; // Ascending order
    }
}