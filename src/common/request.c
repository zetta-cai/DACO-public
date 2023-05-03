#include "common/request.h"

namespace covered
{
    const std::string Request::kClassName("Request");

    Request::Request(const Key& key, const Value& value)
    {
        key_ = key;
        value_ = value;
    }

    Request::~Request() {}

    Key Request::getKey()
    {
        return key_;
    }

    Value Request::getValue()
    {
        return value_;
    }

    uint32_t Request::getMsgPayloadSize()
    {
        // keysize + key + valuesize + value
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length() + sizeof(uint32_t) + value_.getValuesize();
        return msg_payload_size;
    }

    uint32_t Request::serialize(DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t key_serialize_size = key_.serialize(msg_payload);
        size += key_serialize_size;
        uint32_t value_serialize_size = value_.serialize(msg_payload, key_serialize_size);
        size += value_serialize_size;
        return size;
    }

    uint32_t Request::deserialize(const DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload);
        size += key_deserialize_size;
        uint32_t value_deserialize_size = value_.deserialize(msg_payload, key_deserialize_size);
        size += value_deserialize_size;
        return size;
    }
}