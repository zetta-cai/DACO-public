#include "message/key_edgeset_message.h"

namespace covered
{
    const std::string KeyEdgesetMessage::kClassName("KeyEdgesetMessage");

    KeyEdgesetMessage::KeyEdgesetMessage(const Key& key, const Edgeset& edgeset, const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : MessageBase(message_type, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        key_ = key;
        edgeset_ = edgeset;
    }

    KeyEdgesetMessage::KeyEdgesetMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyEdgesetMessage::~KeyEdgesetMessage() {}

    Key KeyEdgesetMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    const Edgeset& KeyEdgesetMessage::getEdgesetRef() const
    {
        checkIsValid_();
        return edgeset_;
    }

    uint32_t KeyEdgesetMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + edgeset payload
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + edgeset_.getEdgesetPayloadSize();
        return msg_payload_size;
    }

    uint32_t KeyEdgesetMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t edgeset_serialize_size = edgeset_.serialize(msg_payload, size);
        size += edgeset_serialize_size;
        return size - position;
    }

    uint32_t KeyEdgesetMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t edgeset_deserialize_size = edgeset_.deserialize(msg_payload, size);
        size += edgeset_deserialize_size;
        return size - position;
    }
}