#include "message/data/local/local_get_response.h"

namespace covered
{
    const std::string LocalGetResponse::kClassName("LocalGetResponse");

    LocalGetResponse::LocalGetResponse(const Key& key, const Value& value, const Hitflag& hitflag) : KeyValueHitflagMessage(key, value, hitflag, MessageType::kLocalGetResponse)
    {
    }

    LocalGetResponse::LocalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    LocalGetResponse::~LocalGetResponse() {}
}