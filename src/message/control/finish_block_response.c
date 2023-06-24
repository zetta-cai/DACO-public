#include "message/control/finish_block_response.h"

namespace covered
{
    const std::string FinishBlockResponse::kClassName("FinishBlockResponse");

    FinishBlockResponse::FinishBlockResponse(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kFinishBlockResponse, source_index)
    {
    }

    FinishBlockResponse::FinishBlockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockResponse::~FinishBlockResponse() {}
}