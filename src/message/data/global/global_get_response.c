#include "message/data/global/global_get_response.h"

namespace covered
{
    const std::string GlobalGetResponse::kClassName("GlobalGetResponse");

    GlobalGetResponse::GlobalGetResponse(const Key& key, const Value& value) : KeyValueMessage(key, value, MessageType::kGlobalGetResponse)
    {
        key_ = key;
        value_ = value;
    }

    GlobalGetResponse::GlobalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    GlobalGetResponse::~GlobalGetResponse() {}
}