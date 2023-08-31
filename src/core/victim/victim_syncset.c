#include "core/victim/victim_syncset.h"

namespace covered
{
    const std::string VictimSyncset::kClassName = "VictimSyncset";

    VictimSyncset::VictimSyncset(const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, DirectoryInfo, KeyHasher>& local_beaconed_victims) : local_synced_victims_(local_synced_victims), local_beaconed_victims_(local_beaconed_victims)
    {
        local_synced_victims_.clear();
        local_beaconed_victims_.clear();
    }

    VictimSyncset::~VictimSyncset() {}

    uint32_t VictimSyncset::getVictimSyncsetPayloadSize() const
    {
        uint32_t victim_syncset_payload_size = 0;

        victim_syncset_payload_size += sizeof(uint32_t); // Size of local_synced_victims_
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); ++cacheinfo_list_iter)
        {
            victim_syncset_payload_size += cacheinfo_list_iter->getVictimCacheinfoPayloadSize();
        }

        victim_syncset_payload_size += sizeof(uint32_t); // Size of local_beaconed_victims_
        for (std::unordered_map<Key, DirectoryInfo, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); ++dirinfo_map_iter)
        {
            victim_syncset_payload_size += dirinfo_map_iter->first.getKeyPayloadSize();
            victim_syncset_payload_size += dirinfo_map_iter->second.getDirectoryInfoPayloadSize();
        }

        return victim_syncset_payload_size;
    }

    uint32_t VictimSyncset::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;

        uint32_t local_synced_victims_size = local_synced_victims_.size();
        msg_payload.deserialize(size, (const char*)&local_synced_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); ++cacheinfo_list_iter)
        {
            uint32_t cacheinfo_serialize_size = cacheinfo_list_iter->serialize(msg_payload, size);
            size += cacheinfo_serialize_size;
        }

        uint32_t local_beaconed_victims_size = local_beaconed_victims_.size();
        msg_payload.deserialize(size, (const char*)&local_beaconed_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        for (std::unordered_map<Key, DirectoryInfo, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); ++dirinfo_map_iter)
        {
            uint32_t key_serialize_size = dirinfo_map_iter->first.serialize(msg_payload, size);
            size += key_serialize_size;
            uint32_t dirinfo_serialize_size = dirinfo_map_iter->second.serialize(msg_payload, size);
            size += dirinfo_serialize_size;
        }

        return size - position;
    }

    uint32_t VictimSyncset::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;

        uint32_t local_synced_victims_size = 0;
        msg_payload.serialize(size, (char*)&local_synced_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        local_synced_victims_.clear();
        for (uint32_t i = 0; i < local_synced_victims_size; ++i)
        {
            VictimCacheinfo cacheinfo;
            uint32_t cacheinfo_deserialize_size = cacheinfo.deserialize(msg_payload, size);
            size += cacheinfo_deserialize_size;
            local_synced_victims_.push_back(cacheinfo);
        }

        uint32_t local_beaconed_victims_size = 0;
        msg_payload.serialize(size, (char*)&local_beaconed_victims_size, sizeof(uint32_t));
        size += sizeof(uint32_t);

        local_beaconed_victims_.clear();
        for (uint32_t i = 0; i < local_beaconed_victims_size; ++i)
        {
            Key key;
            uint32_t key_deserialize_size = key.deserialize(msg_payload, size);
            size += key_deserialize_size;
            DirectoryInfo dirinfo;
            uint32_t dirinfo_deserialize_size = dirinfo.deserialize(msg_payload, size);
            size += dirinfo_deserialize_size;
            local_beaconed_victims_.insert(std::pair(key, dirinfo));
        }

        return size - position;
    }

    const VictimSyncset& VictimSyncset::operator=(const VictimSyncset& other)
    {
        local_synced_victims_ = other.local_synced_victims_;
        local_beaconed_victims_ = other.local_beaconed_victims_;
        
        return *this;
    }
}