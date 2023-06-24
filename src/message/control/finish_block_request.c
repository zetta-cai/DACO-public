#include "message/control/finish_block_request.h"

namespace covered
{
    const std::string FinishBlockRequest::kClassName("FinishBlockRequest");

    FinishBlockRequest::FinishBlockRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kFinishBlockRequest, source_index)
    {
    }

    FinishBlockRequest::FinishBlockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockRequest::~FinishBlockRequest() {}
}