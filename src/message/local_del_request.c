#include "message/local_del_request.h"

namespace covered
{
    const std::string LocalDelRequest::kClassName("LocalDelRequest");

    LocalDelRequest::LocalDelRequest(const Key& key) : MessageBase(MessageType::kLocalDelRequest)
    {
        key_ = key;
    }

    LocalDelRequest::LocalDelRequest(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    LocalDelRequest::~LocalDelRequest() {}

    Key LocalDelRequest::getKey() const
    {
        return key_;
    }

    uint32_t LocalDelRequest::getMsgPayloadSize()
    {
        // keysize + key
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length();
        return msg_payload_size;
    }

    uint32_t LocalDelRequest::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size;
    }

    uint32_t LocalDelRequest::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size;
    }
}