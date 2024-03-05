#include "message/key_two_byte_victimset_message.h"

namespace covered
{
    const std::string KeyTwoByteVictimsetMessage::kClassName("KeyTwoByteVictimsetMessage");

    KeyTwoByteVictimsetMessage::KeyTwoByteVictimsetMessage(const Key& key, const uint8_t& first_byte, const uint8_t& second_byte, const VictimSyncset& victim_syncset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        first_byte_ = first_byte;
        second_byte_ = second_byte;
        victim_syncset_ = victim_syncset;
    }

    KeyTwoByteVictimsetMessage::KeyTwoByteVictimsetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyTwoByteVictimsetMessage::~KeyTwoByteVictimsetMessage() {}

    Key KeyTwoByteVictimsetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint8_t KeyTwoByteVictimsetMessage::getFirstByte_() const
    {
        checkIsValid_();
        return first_byte_;
    }

    uint8_t KeyTwoByteVictimsetMessage::getSecondByte_() const
    {
        checkIsValid_();
        return second_byte_;
    }

    const VictimSyncset& KeyTwoByteVictimsetMessage::getVictimSyncsetRef() const
    {
        checkIsValid_();
        return victim_syncset_;
    }

    uint32_t KeyTwoByteVictimsetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + first byte + second byte + victim syncset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t) + sizeof(uint8_t) + victim_syncset_.getVictimSyncsetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyTwoByteVictimsetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&first_byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        msg_payload.deserialize(size, (const char*)&second_byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_serialize_size = victim_syncset_.serialize(msg_payload, size);
        size += victim_syncset_serialize_size;
        return size - position;
    }

    uint32_t KeyTwoByteVictimsetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&first_byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        msg_payload.serialize(size, (char *)&second_byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        uint32_t victim_syncset_deserialize_size = victim_syncset_.deserialize(msg_payload, size);
        size += victim_syncset_deserialize_size;
        return size - position;
    }
}