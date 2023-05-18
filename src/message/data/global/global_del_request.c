#include "message/data/global/global_del_request.h"

namespace covered
{
    const std::string GlobalDelRequest::kClassName("GlobalDelRequest");

    GlobalDelRequest::GlobalDelRequest(const Key& key) : KeyMessage(key, MessageType::kGlobalDelRequest)
    {
    }

    GlobalDelRequest::GlobalDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelRequest::~GlobalDelRequest() {}
}