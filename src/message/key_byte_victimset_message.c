#include "message/key_byte_victimset_message.h"

namespace covered
{
    const std::string KeyByteVictimsetMessage::kClassName("KeyByteVictimsetMessage");

    KeyByteVictimsetMessage::KeyByteVictimsetMessage(const Key& key, const uint8_t& byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        byte_ = byte;
        victim_syncset_ = victim_syncset;
    }

    KeyByteVictimsetMessage::KeyByteVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyByteVictimsetMessage::~KeyByteVictimsetMessage() {}

    Key KeyByteVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint8_t KeyByteVictimsetMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    const VictimSyncset& KeyByteVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyByteVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + hit flag + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyByteVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyByteVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}