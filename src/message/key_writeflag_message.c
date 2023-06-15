#include "message/key_writeflag_message.h"

namespace covered
{
    const std::string KeyWriteflagMessage::kClassName("KeyWriteflagMessage");

    KeyWriteflagMessage::KeyWriteflagMessage(const Key& key, const bool& is_being_written, const MessageType& message_type) : MessageBase(message_type)
    {
        key_ = key;
        is_being_written_ = is_being_written;
    }

    KeyWriteflagMessage::KeyWriteflagMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyWriteflagMessage::~KeyWriteflagMessage() {}

    Key KeyWriteflagMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    bool KeyWriteflagMessage::isBeingWritten() const
    {
        checkIsValid_();
        return is_being_written_;
    }

    uint32_t KeyWriteflagMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + is being written
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(bool);
        return msg_payload_size;
    }

    uint32_t KeyWriteflagMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&is_being_written_, sizeof(bool));
        size += sizeof(bool);
        return size - position;
    }

    uint32_t KeyWriteflagMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&is_being_written_, sizeof(bool));
        size += sizeof(bool);
        return size - position;
    }
}