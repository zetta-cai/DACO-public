#include "core/victim/victim_syncset.h"

namespace covered
{
    const std::string VictimSyncset::kClassName = "VictimSyncset";

    const uint8_t VictimSyncset::INVALID_BITMAP = 0b11111111;
    const uint8_t VictimSyncset::COMPLETE_BITMAP = 0b00000000;
    const uint8_t VictimSyncset::CACHE_MARGIN_BYTES_DELTA_MASK = 0b00000011;
    const uint8_t VictimSyncset::LOCAL_SYNCED_VICTIMS_DEDUP_MASK = 0b00000101;
    const uint8_t VictimSyncset::LOCAL_BEACONED_VICTIMS_DEDUP_MASK = 0b00001001;

    // TODO: END HERE

    VictimSyncset::VictimSyncset() : compressed_bitmap_(INVALID_BITMAP), cache_margin_bytes_(0), cache_margin_delta_bytes_(0)
    {
        local_synced_victims_.clear();
        local_beaconed_victims_.clear();
    }

    VictimSyncset::VictimSyncset(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims) : cache_margin_bytes_(cache_margin_bytes)
    {
        compressed_bitmap_ = COMPLETE_BITMAP;
        cache_margin_delta_bytes_ = 0;
        local_synced_victims_ = local_synced_victims;
        local_beaconed_victims_ = local_beaconed_victims;

        assertAllCacheinfosComplete_();
        assertAllDirinfoSetsComplete_();
    }

    VictimSyncset::~VictimSyncset() {}

    bool VictimSyncset::getCacheMarginBytesOrDelta(uint64_t& cache_margin_bytes, uint32_t cache_margin_delta_bytes) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        if (!with_complete_cache_margin_bytes)
        {
            cache_margin_delta_bytes = cache_margin_delta_bytes_;
        }
        else
        {
            cache_margin_bytes = cache_margin_bytes_;
        }

        return with_complete_cache_margin_bytes;
    }

    bool VictimSyncset::getLocalSyncedVictims(std::list<VictimCacheinfo>& local_synced_vitims) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) != LOCAL_SYNCED_VICTIMS_DEDUP_MASK);
        if (!with_complete_local_synced_victims) // At least one victim cacheinfo is deduped
        {
            assertAtLeastOneCacheinfoDeduped_();
        }
        else // All victim cacheinfos should be complete
        {
            assertAllCacheinfosComplete_();
        }

        local_synced_vitims = local_synced_victims_;

        return with_complete_local_synced_victims;
    }

    bool VictimSyncset::getLocalBeaconedVictims(std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) != LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        if (!with_complete_local_beaconed_victims) // At least one dirinfo set is deduped
        {
            assertAtLeastOneDirinfoSetCompressed_();
        }
        else // All dirinfo sets should be complete
        {
            assertAllDirinfoSetsComplete_();
        }

        local_beaconed_victims = local_beaconed_victims_;

        return with_complete_local_beaconed_victims;
    }

    uint32_t VictimSyncset::getVictimSyncsetPayloadSize() const
    {
        // TODO: we can tune the sizes of local synched/beaconed victims, as the numbers are limited under our design (at most peredge_synced_victimcnt and peredge_synced_victimcnt * edgecnt)

        // TODO: END HERE

        uint32_t victim_syncset_payload_size = 0;

        // Cache margin bytes
        victim_syncset_payload_size += sizeof(uint64_t);

        // Local synced victims
        victim_syncset_payload_size += sizeof(uint32_t); // Size of local_synced_victims_
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); cacheinfo_list_iter++)
        {
            victim_syncset_payload_size += cacheinfo_list_iter->getVictimCacheinfoPayloadSize();
        }

        // Local beaconed victims
        victim_syncset_payload_size += sizeof(uint32_t); // Size of local_beaconed_victims_
        for (std::unordered_map<Key, dirinfo_set_t, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); dirinfo_map_iter++)
        {
            victim_syncset_payload_size += dirinfo_map_iter->first.getKeyPayloadSize();

            // Dirinfo set for the given key
            const dirinfo_set_t& tmp_dirinfo_set = dirinfo_map_iter->second;
            victim_syncset_payload_size += sizeof(uint32_t); // Size of tmp_dirinfo_set
            for (dirinfo_set_t::const_iterator dirinfo_set_iter = tmp_dirinfo_set.begin(); dirinfo_set_iter != tmp_dirinfo_set.end(); dirinfo_set_iter++)
            {
                victim_syncset_payload_size += dirinfo_set_iter->getDirectoryInfoPayloadSize();
            }
        }

        return victim_syncset_payload_size;
    }

    uint32_t VictimSyncset::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;

        // Cache margin bytes
        msg_payload.deserialize(size, (const char*)&cache_margin_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);

        // Size of local synced victims
        uint32_t local_synced_victims_size = local_synced_victims_.size();
        msg_payload.deserialize(size, (const char*)&local_synced_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Local synced victims
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); cacheinfo_list_iter++)
        {
            uint32_t cacheinfo_serialize_size = cacheinfo_list_iter->serialize(msg_payload, size);
            size += cacheinfo_serialize_size;
        }

        // Size of local beaconed victims
        uint32_t local_beaconed_victims_size = local_beaconed_victims_.size();
        msg_payload.deserialize(size, (const char*)&local_beaconed_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Local beaconed victims
        for (std::unordered_map<Key, dirinfo_set_t, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); dirinfo_map_iter++)
        {
            uint32_t key_serialize_size = dirinfo_map_iter->first.serialize(msg_payload, size);
            size += key_serialize_size;

            // Size of dirinfo set for the given key
            const dirinfo_set_t& tmp_dirinfo_set = dirinfo_map_iter->second;
            uint32_t tmp_dirinfo_set_size = tmp_dirinfo_set.size();
            msg_payload.deserialize(size, (const char*)&tmp_dirinfo_set_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Dirinfo set for the given key
            for (dirinfo_set_t::const_iterator dirinfo_set_iter = tmp_dirinfo_set.begin(); dirinfo_set_iter != tmp_dirinfo_set.end(); dirinfo_set_iter++)
            {
                uint32_t dirinfo_serialize_size = dirinfo_set_iter->serialize(msg_payload, size);
                size += dirinfo_serialize_size;
            }
        }

        return size - position;
    }

    uint32_t VictimSyncset::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;

        // Cache margin bytes
        msg_payload.serialize(size, (char*)&cache_margin_bytes_, sizeof(uint64_t));
        size += sizeof(uint64_t);

        // Size of local synced victims
        uint32_t local_synced_victims_size = 0;
        msg_payload.serialize(size, (char*)&local_synced_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Local synced victims
        local_synced_victims_.clear();
        for (uint32_t i = 0; i < local_synced_victims_size; i++)
        {
            VictimCacheinfo cacheinfo;
            uint32_t cacheinfo_deserialize_size = cacheinfo.deserialize(msg_payload, size);
            size += cacheinfo_deserialize_size;
            local_synced_victims_.push_back(cacheinfo);
        }

        // Size of local beaconed victims
        uint32_t local_beaconed_victims_size = 0;
        msg_payload.serialize(size, (char*)&local_beaconed_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        // Local beaconed victims
        local_beaconed_victims_.clear();
        for (uint32_t i = 0; i < local_beaconed_victims_size; i++)
        {
            Key key;
            uint32_t key_deserialize_size = key.deserialize(msg_payload, size);
            size += key_deserialize_size;

            // Size of dirinfo set for the given key
            uint32_t tmp_dirinfo_set_size = 0;
            msg_payload.serialize(size, (char*)&tmp_dirinfo_set_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Dirinfo set for the given key
            dirinfo_set_t tmp_dirinfo_set;
            for (uint32_t j = 0; j < tmp_dirinfo_set_size; j++)
            {
                DirectoryInfo tmp_dirinfo;
                uint32_t dirinfo_deserialize_size = tmp_dirinfo.deserialize(msg_payload, size);
                size += dirinfo_deserialize_size;

                tmp_dirinfo_set.insert(tmp_dirinfo);
            }

            local_beaconed_victims_.insert(std::pair(key, tmp_dirinfo_set));
        }

        return size - position;
    }

    const VictimSyncset& VictimSyncset::operator=(const VictimSyncset& other)
    {
        cache_margin_bytes_ = other.cache_margin_bytes_;
        local_synced_victims_ = other.local_synced_victims_;
        local_beaconed_victims_ = other.local_beaconed_victims_;
        
        return *this;
    }

    void VictimSyncset::assertAtLeastOneCacheinfoDeduped_() const
    {
        bool tmp_at_least_one_dedup = false;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = local_synced_victims_.begin(); cacheinfo_list_const_iter != local_synced_victims_.end(); cacheinfo_list_const_iter++)
        {
            if (cacheinfo_list_const_iter->isDeduped())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);

        return;
    }

    void VictimSyncset::assertAllCacheinfosComplete_() const
    {
        bool tmp_all_complete = true;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = local_synced_victims_.begin(); cacheinfo_list_const_iter != local_synced_victims_.end(); cacheinfo_list_const_iter++)
        {
            if (!cacheinfo_list_const_iter->isComplete())
            {
                tmp_all_complete = false;
                break;
            }
        }
        assert(tmp_all_complete == true);

        return;
    }

    void VictimSyncset::assertAtLeastOneDirinfoSetCompressed_() const
    {
        bool tmp_at_least_one_dedup = false;
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_const_iter = local_beaconed_victims_.begin(); dirinfo_map_const_iter != local_beaconed_victims_.end(); dirinfo_map_const_iter++)
        {
            if (dirinfo_map_const_iter->second.isCompressed())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);

        return;
    }

    void VictimSyncset::assertAllDirinfoSetsComplete_() const
    {
        bool tmp_all_complete = true;
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_const_iter = local_beaconed_victims_.begin(); dirinfo_map_const_iter != local_beaconed_victims_.end(); dirinfo_map_const_iter++)
        {
            if (!dirinfo_map_const_iter->second.isComplete())
            {
                tmp_all_complete = false;
                break;
            }
        }
        assert(tmp_all_complete == true);

        return;
    }
}