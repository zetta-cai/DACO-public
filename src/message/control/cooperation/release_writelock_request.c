#include "message/control/cooperation/release_writelock_request.h"

namespace covered
{
    const std::string ReleaseWritelockRequest::kClassName("ReleaseWritelockRequest");

    ReleaseWritelockRequest::ReleaseWritelockRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kReleaseWritelockRequest, source_index, source_addr, EventList())
    {
    }

    ReleaseWritelockRequest::ReleaseWritelockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockRequest::~ReleaseWritelockRequest() {}
}