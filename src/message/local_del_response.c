#include "message/local_del_response.h"

namespace covered
{
    const std::string LocalDelResponse::kClassName("LocalDelResponse");

    LocalDelResponse::LocalDelResponse(const Key& key) : MessageBase(MessageType::kLocalDelResponse)
    {
        key_ = key;
    }

    LocalDelResponse::LocalDelResponse(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    LocalDelResponse::~LocalDelResponse() {}

    Key LocalDelResponse::getKey() const
    {
        return key_;
    }

    uint32_t LocalDelResponse::getMsgPayloadSize()
    {
        // keysize + key
        uint32_t msg_payload_size = sizeof(uint32_t) + key_.getKeystr().length();
        return msg_payload_size;
    }

    uint32_t LocalDelResponse::serializeInternal_(DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_serialize_size = key_.serialize(msg_payload, size);
        size += key_serialize_size;
        return size;
    }

    uint32_t LocalDelResponse::deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t key_deserialize_size = key_.deserialize(msg_payload, size);
        size += key_deserialize_size;
        return size;
    }
}