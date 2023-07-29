#include "message/key_message.h"

namespace covered
{
    const std::string KeyMessage::kClassName("KeyMessage");

    KeyMessage::KeyMessage(const Key& key, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, event_list, skip_propagation_latency)
    {
        key_ = key;
    }

    KeyMessage::KeyMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyMessage::~KeyMessage() {}

    Key KeyMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint32_t KeyMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size - position;
    }

    uint32_t KeyMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size - position;
    }
}