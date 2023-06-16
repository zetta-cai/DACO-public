#include "message/control/finish_block_request.h"

namespace covered
{
    const std::string FinishBlockRequest::kClassName("FinishBlockRequest");

    FinishBlockRequest::FinishBlockRequest(const Key& key) : KeyMessage(key, MessageType::kFinishBlockRequest)
    {
    }

    FinishBlockRequest::FinishBlockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockRequest::~FinishBlockRequest() {}
}