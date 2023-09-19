#include "core/victim/victim_syncset.h"

namespace covered
{
    const std::string VictimSyncset::kClassName = "VictimSyncset";

    VictimSyncset::VictimSyncset(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_victims) : cache_margin_bytes_(cache_margin_bytes), local_synced_victims_(local_synced_victims), local_beaconed_victims_(local_beaconed_victims)
    {
        local_synced_victims_.clear();
        local_beaconed_victims_.clear();
    }

    VictimSyncset::~VictimSyncset() {}

    uint64_t VictimSyncset::getCacheMarginBytes() const
    {
        return cache_margin_bytes_;
    }

    const std::list<VictimCacheinfo>& VictimSyncset::getLocalSyncedVictimsRef() const
    {
        return local_synced_victims_;
    }

    const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& VictimSyncset::getLocalBeaconedVictimsRef() const
    {
        return local_beaconed_victims_;
    }

    uint32_t VictimSyncset::getVictimSyncsetPayloadSize() const
    {
        // TODO: we can tune the sizes of local synched/beaconed victims, as the numbers are limited under our design (at most peredge_synced_victimcnt and peredge_synced_victimcnt * edgecnt)

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
}