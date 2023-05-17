#include "message/local_put_request.h"

namespace covered
{
    const std::string LocalPutRequest::kClassName("LocalPutRequest");

    LocalPutRequest::LocalPutRequest(const Key& key, const Value& value) : MessageBase(MessageType::kLocalLocalPutRequest)
    {
        key_ = key;
        value_ = value;
    }

    LocalPutRequest::LocalPutRequest(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    LocalPutRequest::~LocalPutRequest() {}

    Key LocalPutRequest::getKey() const
    {
        return key_;
    }

    Value LocalPutRequest::getValue() const
    {
        return value_;
    }

    uint32_t LocalPutRequest::getMsgPayloadSize()
    {
        // keysize + key + valuesize + value
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length() + sizeof(uint32_t) + value_.getValuesize();
        return msg_payload_size;
    }

    uint32_t LocalPutRequest::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, size);
        size += value_serialize_size;
        return size;
    }

    uint32_t LocalPutRequest::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, size);
        size += value_deserialize_size;
        return size;
    }
}