#include "message/control/finish_block_response.h"

namespace covered
{
    const std::string FinishBlockResponse::kClassName("FinishBlockResponse");

    FinishBlockResponse::FinishBlockResponse(const Key& key) : KeyMessage(key, MessageType::kFinishBlockResponse)
    {
    }

    FinishBlockResponse::FinishBlockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockResponse::~FinishBlockResponse() {}
}