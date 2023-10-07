#include "message/key_value_directory_victimset_edgeset_message.h"

namespace covered
{
    const std::string KeyValueDirectoryVictimsetEdgesetMessage::kClassName("KeyValueDirectoryVictimsetEdgesetMessage");

    KeyValueDirectoryVictimsetEdgesetMessage::KeyValueDirectoryVictimsetEdgesetMessage(const Key& key, const Value& value, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        directory_info_ = directory_info;
        victim_syncset_ = victim_syncset;
        edgeset_ = edgeset;
    }

    KeyValueDirectoryVictimsetEdgesetMessage::KeyValueDirectoryVictimsetEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueDirectoryVictimsetEdgesetMessage::~KeyValueDirectoryVictimsetEdgesetMessage() {}

    Key KeyValueDirectoryVictimsetEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueDirectoryVictimsetEdgesetMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    DirectoryInfo KeyValueDirectoryVictimsetEdgesetMessage::getDirectoryInfo() const
    {
        checkIsValid_();
        return directory_info_;
    }

    const VictimSyncset& KeyValueDirectoryVictimsetEdgesetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    const Edgeset& KeyValueDirectoryVictimsetEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyValueDirectoryVictimsetEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + directory info (target edge idx) + victim syncset payload + edgeset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + directory_info_.getDirectoryInfoPayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueDirectoryVictimsetEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint32_t directory_info_serialize_size = directory_info_.serialize(msg_payload, size);
        size += directory_info_serialize_size;
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyValueDirectoryVictimsetEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint32_t directory_info_deserialize_size = directory_info_.deserialize(msg_payload, size);
        size += directory_info_deserialize_size;
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}