#include "message/key_hitflag_message.h"

namespace covered
{
    const std::string KeyHitflagMessage::kClassName("KeyHitflagMessage");

    KeyHitflagMessage::KeyHitflagMessage(const Key& key, const Hitflag& hitflag, const MessageType& message_type) : MessageBase(message_type)
    {
        key_ = key;
        hitflag_ = hitflag;
    }

    KeyHitflagMessage::KeyHitflagMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyHitflagMessage::~KeyHitflagMessage() {}

    Key KeyHitflagMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Hitflag KeyHitflagMessage::getHitflag() const
    {
        checkIsValid_();
        return hitflag_;
    }

    uint32_t KeyHitflagMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + hit flag
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t);
        return msg_payload_size;
    }

    uint32_t KeyHitflagMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint8_t hitflag_value = static_cast<uint8_t>(hitflag_);
        msg_payload.deserialize(size, (const char*)&hitflag_value, sizeof(uint8_t));
        size += sizeof(uint8_t);
        return size - position;
    }

    uint32_t KeyHitflagMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint8_t hitflag_value = 0;
        msg_payload.serialize(size, (char *)&hitflag_value, sizeof(uint8_t));
        hitflag_ = static_cast<Hitflag>(hitflag_value);
        size += sizeof(uint8_t);
        return size - position;
    }
}