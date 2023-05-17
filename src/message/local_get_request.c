#include "message/local_get_request.h"

namespace covered
{
    const std::string LocalGetRequest::kClassName("LocalGetRequest");

    LocalGetRequest::LocalGetRequest(const Key& key) : MessageBase(MessageType::kLocalGetRequest)
    {
        key_ = key;
    }

    LocalGetRequest::LocalGetRequest(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    LocalGetRequest::~LocalGetRequest() {}

    Key LocalGetRequest::getKey() const
    {
        return key_;
    }

    uint32_t LocalGetRequest::getMsgPayloadSize()
    {
        // keysize + key
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length();
        return msg_payload_size;
    }

    uint32_t LocalGetRequest::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size;
    }

    uint32_t LocalGetRequest::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size;
    }
}