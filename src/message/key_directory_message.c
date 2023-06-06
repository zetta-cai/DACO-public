#include "message/key_directory_message.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string KeyDirectoryMessage::kClassName("KeyDirectoryMessage");

    KeyDirectoryMessage::KeyDirectoryMessage(const Key& key, const uint32_t& neighbor_edge_idx, const MessageType& message_type) : MessageBase(message_type)
    {
        key_ = key;
        neighbor_edge_idx_ = neighbor_edge_idx;
    }

    KeyDirectoryMessage::KeyDirectoryMessage(const DynamicArray& msg_payload) : MessageBase()
    {
        deserialize(msg_payload);
    }

    KeyDirectoryMessage::~KeyDirectoryMessage() {}

    Key KeyDirectoryMessage::getKey() const
    {
        checkIsValid_();
        return key_;
    }

    uint32_t KeyDirectoryMessage::getNeighborEdgeIdx() const
    {
        checkIsValid_();
        return neighbor_edge_idx_;
    }

    uint32_t KeyDirectoryMessage::getMsgPayloadSizeInternal_() const
    {
        // key payload + neighbor edge idx
        uint32_t msg_payload_size = key_.getKeyPayloadSize() + sizeof(uint32_t);
        return msg_payload_size;
    }

    uint32_t KeyDirectoryMessage::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t bigendian_neighbor_edge_idx = htonl(neighbor_edge_idx_);
        msg_payload.deserialize(size, (const char*)&bigendian_neighbor_edge_idx, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t KeyDirectoryMessage::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t bigendian_neighbor_edge_idx = 0;
        msg_payload.serialize(size, (char *)&bigendian_neighbor_edge_idx, sizeof(uint32_t));
        neighbor_edge_idx_ = ntohl(bigendian_neighbor_edge_idx);
        size += sizeof(uint32_t);
        return size - position;
    }
}