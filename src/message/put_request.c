#include "message/put_request.h"

namespace covered
{
    const std::string PutRequest::kClassName("PutRequest");

    PutRequest::PutRequest(const Key& key, const Value& value) : MessageBase(MessageType::kLocalPutRequest)
    {
        key_ = key;
        value_ = value;
    }

    PutRequest::~PutRequest() {}

    Key PutRequest::getKey()
    {
        return key_;
    }

    Value PutRequest::getValue()
    {
        return value_;
    }

    uint32_t PutRequest::getMsgPayloadSize()
    {
        // keysize + key + valuesize + value
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length() + sizeof(uint32_t) + value_.getValuesize();
        return msg_payload_size;
    }

    uint32_t PutRequest::serialize(DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t key_serialize_size = key_.serialize(msg_payload);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, key_serialize_size);
        size += value_serialize_size;
        return size;
    }

    uint32_t PutRequest::deserialize(const DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, key_deserialize_size);
        size += value_deserialize_size;
        return size;
    }
}