#include "message/data/local/local_get_response.h"

namespace covered
{
    const std::string LocalGetResponse::kClassName("LocalGetResponse");

    LocalGetResponse::LocalGetResponse(const Key& key, const Value& value) : KeyValueMessage(key, value, MessageType::kLocalGetResponse)
    {
        key_ = key;
        value_ = value;
    }

    LocalGetResponse::LocalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    LocalGetResponse::~LocalGetResponse() {}
}