#include "message/control/release_writelock_request.h"

namespace covered
{
    const std::string ReleaseWritelockRequest::kClassName("ReleaseWritelockRequest");

    ReleaseWritelockRequest::ReleaseWritelockRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kReleaseWritelockRequest, source_index)
    {
    }

    ReleaseWritelockRequest::ReleaseWritelockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockRequest::~ReleaseWritelockRequest() {}
}