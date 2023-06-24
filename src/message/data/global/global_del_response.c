#include "message/data/global/global_del_response.h"

namespace covered
{
    const std::string GlobalDelResponse::kClassName("GlobalDelResponse");

    GlobalDelResponse::GlobalDelResponse(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kGlobalDelResponse, source_index)
    {
    }

    GlobalDelResponse::GlobalDelResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelResponse::~GlobalDelResponse() {}
}