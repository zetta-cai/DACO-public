#include "message/data/global/global_put_response.h"

namespace covered
{
    const std::string GlobalPutResponse::kClassName("GlobalPutResponse");

    GlobalPutResponse::GlobalPutResponse(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kGlobalPutResponse, source_index)
    {
    }

    GlobalPutResponse::GlobalPutResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalPutResponse::~GlobalPutResponse() {}
}