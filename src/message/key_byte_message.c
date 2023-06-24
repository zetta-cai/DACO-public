#include "message/key_byte_message.h"

namespace covered
{
    const std::string KeyByteMessage::kClassName("KeyByteMessage");

    KeyByteMessage::KeyByteMessage(const Key& key, const uint8_t& byte, const MessageType& message_type, const uint32_t& source_index) : MessageBase(message_type, source_index)
    {
        key_ = key;
        byte_ = byte;
    }

    KeyByteMessage::KeyByteMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyByteMessage::~KeyByteMessage() {}

    Key KeyByteMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint8_t KeyByteMessage::getByte_() const
    {
        checkIsValid_();
        return byte_;
    }

    uint32_t KeyByteMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + hit flag
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint8_t);
        return msg_payload_size;
    }

    uint32_t KeyByteMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        msg_payload.deserialize(size, (const char*)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        return size - position;
    }

    uint32_t KeyByteMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        msg_payload.serialize(size, (char *)&byte_, sizeof(uint8_t));
        size += sizeof(uint8_t);
        return size - position;
    }
}