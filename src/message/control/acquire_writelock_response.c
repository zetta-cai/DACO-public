#include "message/control/acquire_writelock_response.h"

namespace covered
{
    const std::string AcquireWritelockResponse::kClassName("AcquireWritelockResponse");

    AcquireWritelockResponse::AcquireWritelockResponse(const Key& key) : KeyMessage(key, MessageType::kAcquireWritelockResponse)
    {
    }

    AcquireWritelockResponse::AcquireWritelockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    AcquireWritelockResponse::~AcquireWritelockResponse() {}
}