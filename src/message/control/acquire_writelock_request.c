#include "message/control/acquire_writelock_request.h"

namespace covered
{
    const std::string AcquireWritelockRequest::kClassName("AcquireWritelockRequest");

    AcquireWritelockRequest::AcquireWritelockRequest(const Key& key) : KeyMessage(key, MessageType::kAcquireWritelockRequest)
    {
    }

    AcquireWritelockRequest::AcquireWritelockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    AcquireWritelockRequest::~AcquireWritelockRequest() {}
}