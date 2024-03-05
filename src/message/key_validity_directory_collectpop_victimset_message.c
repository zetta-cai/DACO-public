#include "message/key_validity_directory_collectpop_victimset_message.h"

#include <assert.h>

namespace covered
{
    const std::string KeyValidityDirectoryCollectpopVictimsetMessage::kClassName("KeyValidityDirectoryCollectpopVictimsetMessage");

    KeyValidityDirectoryCollectpopVictimsetMessage::KeyValidityDirectoryCollectpopVictimsetMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr), collected_popularity_()
    {
        assert(is_valid_directory_exist == true);
        key_ = key;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
        victim_syncset_ = victim_syncset;
    }

    KeyValidityDirectoryCollectpopVictimsetMessage::KeyValidityDirectoryCollectpopVictimsetMessage(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        assert(is_valid_directory_exist == false);
        key_ = key;
        is_valid_directory_exist_ = is_valid_directory_exist;
        directory_info_ = directory_info;
        collected_popularity_ = collected_popularity;
        victim_syncset_ = victim_syncset;
    }

    KeyValidityDirectoryCollectpopVictimsetMessage::KeyValidityDirectoryCollectpopVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValidityDirectoryCollectpopVictimsetMessage::~KeyValidityDirectoryCollectpopVictimsetMessage() {}

    Key KeyValidityDirectoryCollectpopVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyValidityDirectoryCollectpopVictimsetMessage::isValidDirectoryExist() const
    {
        checkIsValid_();
        return is_valid_directory_exist_;
    }

    DirectoryInfo KeyValidityDirectoryCollectpopVictimsetMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    const CollectedPopularity& KeyValidityDirectoryCollectpopVictimsetMessage::getCollectedPopularityRef() const
    {
        checkIsValid_();
        assert(is_valid_directory_exist_ == false);
        return collected_popularity_;
    }

    const VictimSyncset& KeyValidityDirectoryCollectpopVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyValidityDirectoryCollectpopVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        uint32_t msg_payload_size = 0;
        if (is_valid_directory_exist_) // Admit a directory info
        {
            // key payload + is cooperatively cached + directory info (target edge idx) + victim syncset
            msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize();
        }
        else // Evict a directory info
        {
            // key payload + is cooperatively cached + directory info (target edge idx) + collected popularity + victim syncset
            msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool) + directory_info_.getDirectoryInfoPayloadSize() + collected_popularity_.getCollectedPopularityPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize();
        }
        return msg_payload_size;
    }

    uint32_t KeyValidityDirectoryCollectpopVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_serialize_size = directory_info_.serialize(msg_payload, size);
        size += directory_info_serialize_size;
        if (!is_valid_directory_exist_) // Evict a directory info
        {
            uint32_t collected_popularity_serialize_size = collected_popularity_.serialize(msg_payload, size);
            size += collected_popularity_serialize_size;
        }
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyValidityDirectoryCollectpopVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&is_valid_directory_exist_, sizeof(bool));
        size += sizeof(bool);
        uint32_t directory_info_deserialize_size = directory_info_.deserialize(msg_payload, size);
        size += directory_info_deserialize_size;
        if (!is_valid_directory_exist_) // Evict a directory info
        {
            uint32_t collected_popularity_deserialize_size = collected_popularity_.deserialize(msg_payload, size);
            size += collected_popularity_deserialize_size;
        }
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}