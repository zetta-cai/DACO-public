#include "message/data/global/global_del_response.h"

namespace covered
{
    const std::string GlobalDelResponse::kClassName("GlobalDelResponse");

    GlobalDelResponse::GlobalDelResponse(const Key& key) : KeyMessage(key, MessageType::kGlobalDelResponse)
    {
    }

    GlobalDelResponse::GlobalDelResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelResponse::~GlobalDelResponse() {}
}