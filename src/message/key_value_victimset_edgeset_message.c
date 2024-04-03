#include "message/key_value_victimset_edgeset_message.h"

namespace covered
{
    const std::string KeyValueVictimsetEdgesetMessage::kClassName("KeyValueVictimsetEdgesetMessage");

    KeyValueVictimsetEdgesetMessage::KeyValueVictimsetEdgesetMessage(const Key& key, const Value& value, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        value_ = value;
        victim_syncset_ = victim_syncset;
        edgeset_ = edgeset;
    }

    KeyValueVictimsetEdgesetMessage::KeyValueVictimsetEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueVictimsetEdgesetMessage::~KeyValueVictimsetEdgesetMessage() {}

    Key KeyValueVictimsetEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueVictimsetEdgesetMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    const VictimSyncset& KeyValueVictimsetEdgesetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    const Edgeset& KeyValueVictimsetEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyValueVictimsetEdgesetMessage::getVictimSyncsetBytes() const
    {
        checkIsValid_();
        return victim_syncset_.getVictimSyncsetPayloadSize();
    }

    uint32_t KeyValueVictimsetEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + victim syncset payload + edgeset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + victim_syncset_.getVictimSyncsetPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueVictimsetEdgesetMessage::getMsgBandwidthSizeInternal_() const
    {
        // key payload + ideal value content ize + victim syncset payload + edgeset payload
        uint32_t msg_bandwidth_size = key_.getKeyPayloadSize() + value_.getValuesize() + victim_syncset_.getVictimSyncsetPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_bandwidth_size;
    }

    uint32_t KeyValueVictimsetEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyValueVictimsetEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}