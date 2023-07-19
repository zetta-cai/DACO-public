#include "message/key_value_message.h"

namespace covered
{
    const std::string KeyValueMessage::kClassName("KeyValueMessage");

    KeyValueMessage::KeyValueMessage(const Key& key, const Value& value, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : MessageBase(message_type, source_index, source_addr, event_list)
    {
        key_ = key;
        value_ = value;
    }

    KeyValueMessage::KeyValueMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueMessage::~KeyValueMessage() {}

    Key KeyValueMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    uint32_t KeyValueMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        return size - position;
    }

    uint32_t KeyValueMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        return size - position;
    }
}