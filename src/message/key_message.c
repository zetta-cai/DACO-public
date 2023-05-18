#include "message/key_message.h"

namespace covered
{
    const std::string KeyMessage::kClassName("KeyMessage");

    KeyMessage::KeyMessage(const Key& key, const MessageType& message_type) : MessageBase(message_type)
    {
        key_ = key;
    }

    KeyMessage::KeyMessage(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    KeyMessage::~KeyMessage() {}

    Key KeyMessage::getKey() const
    {
        return key_;
    }

    uint32_t KeyMessage::getMsgPayloadSize()
    {
        // keysize + key
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length();
        return msg_payload_size;
    }

    uint32_t KeyMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size;
    }

    uint32_t KeyMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size;
    }
}