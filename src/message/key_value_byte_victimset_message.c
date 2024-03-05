#include "message/key_value_byte_victimset_message.h"

namespace covered
{
    const std::string KeyValueByteVictimsetMessage::kClassName("KeyValueByteVictimsetMessage");

    KeyValueByteVictimsetMessage::KeyValueByteVictimsetMessage(const Key& key, const Value& value, const uint8_t& byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        value_ = value;
        byte_ = byte;
        victim_syncset_ = victim_syncset;
    }

    KeyValueByteVictimsetMessage::KeyValueByteVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueByteVictimsetMessage::~KeyValueByteVictimsetMessage() {}

    Key KeyValueByteVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueByteVictimsetMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    uint8_t KeyValueByteVictimsetMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    const VictimSyncset& KeyValueByteVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyValueByteVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + byte + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueByteVictimsetMessage::getMsgBandwidthSizeInternal_() const
    {
        // key payload + ideal value content size + byte + victim syncset payload
        uint32_t msg_bandwidth_size = key_.getKeyPayloadSize() + value_.getValuesize() + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_bandwidth_size;
    }

    uint32_t KeyValueByteVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyValueByteVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}