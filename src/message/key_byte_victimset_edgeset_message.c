#include "message/key_byte_victimset_edgeset_message.h"

namespace covered
{
    const std::string KeyByteVictimsetEdgesetMessage::kClassName("KeyByteVictimsetEdgesetMessage");

    KeyByteVictimsetEdgesetMessage::KeyByteVictimsetEdgesetMessage(const Key& key, const uint8_t& byte, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        byte_ = byte;
        victim_syncset_ = victim_syncset;
        edgeset_ = edgeset;
    }

    KeyByteVictimsetEdgesetMessage::KeyByteVictimsetEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyByteVictimsetEdgesetMessage::~KeyByteVictimsetEdgesetMessage() {}

    Key KeyByteVictimsetEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint8_t KeyByteVictimsetEdgesetMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    const VictimSyncset& KeyByteVictimsetEdgesetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    const Edgeset& KeyByteVictimsetEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyByteVictimsetEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + hit flag + victim syncset payload + edgeset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyByteVictimsetEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyByteVictimsetEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}