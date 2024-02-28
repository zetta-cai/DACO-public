#include "message/key_value_edgeset_message.h"

namespace covered
{
    const std::string KeyValueEdgesetMessage::kClassName("KeyValueEdgesetMessage");

    KeyValueEdgesetMessage::KeyValueEdgesetMessage(const Key& key, const Value& value, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        edgeset_ = edgeset;
    }

    KeyValueEdgesetMessage::KeyValueEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyValueEdgesetMessage::~KeyValueEdgesetMessage() {}

    Key KeyValueEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    Value KeyValueEdgesetMessage::getValue() const
    {
        checkIsValid_();
        return value_;
    }

    const Edgeset& KeyValueEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyValueEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + value payload + edgeset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + value_.getValuePayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyValueEdgesetMessage::getMsgBandwidthSizeInternal_() const
    {
        // key payload + ideal value content size + edgeset payload
        uint32_t msg_bandwidth_size = key_.getKeyPayloadSize() + value_.getValuesize() + edgeset_.getEdgesetPayloadSize();
        return msg_bandwidth_size;
    }

    uint32_t KeyValueEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyValueEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}