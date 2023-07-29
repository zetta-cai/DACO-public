#include "message/control/cooperation/acquire_writelock_request.h"

namespace covered
{
    const std::string AcquireWritelockRequest::kClassName("AcquireWritelockRequest");

    AcquireWritelockRequest::AcquireWritelockRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kAcquireWritelockRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    AcquireWritelockRequest::AcquireWritelockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    AcquireWritelockRequest::~AcquireWritelockRequest() {}
}