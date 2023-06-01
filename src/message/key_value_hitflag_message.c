#include "message/key_value_hitflag_message.h"

namespace covered
{
    const std::string KeyValueHitflagMessage::kClassName("KeyValueHitflagMessage");

    KeyValueHitflagMessage::KeyValueHitflagMessage(const Key& key, const Value& value, const Hitflag& hitflag, const MessageType& message_type) : MessageBase(message_type)
    {
        key_ = key;
        value_ = value;
        hitflag_ = hitflag;
    }

    KeyValueHitflagMessage::KeyValueHitflagMessage(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    KeyValueHitflagMessage::~KeyValueHitflagMessage() {}

    Key KeyValueHitflagMessage::getKey() const
    {
        return key_;
    }

    Value KeyValueHitflagMessage::getValue() const
    {
        return value_;
    }

    Hitflag KeyValueHitflagMessage::getHitflag() const
    {
        return hitflag_;
    }

    uint32_t KeyValueHitflagMessage::getMsgPayloadSizeInternal_() const
    {
        // keysize + key + valuesize + value + hit flag
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length() + sizeof(uint32_t) + value_.getValuesize() + sizeof(uint8_t);
        return msg_payload_size;
    }

    uint32_t KeyValueHitflagMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint8_t hitflag_value = static_cast<uint8_t>(hitflag_);
        msg_payload.deserialize(size, (const char*)&hitflag_value, sizeof(uint8_t));
        size += sizeof(uint8_t);
        return size - position;
    }

    uint32_t KeyValueHitflagMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint8_t hitflag_value = 0;
        msg_payload.serialize(size, (char *)&hitflag_value, sizeof(uint8_t));
        hitflag_ = static_cast<Hitflag>(hitflag_value);
        size += sizeof(uint8_t);
        return size - position;
    }
}