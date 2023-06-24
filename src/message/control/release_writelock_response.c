#include "message/control/release_writelock_response.h"

namespace covered
{
    const std::string ReleaseWritelockResponse::kClassName("ReleaseWritelockResponse");

    ReleaseWritelockResponse::ReleaseWritelockResponse(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kReleaseWritelockResponse, source_index)
    {
    }

    ReleaseWritelockResponse::ReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockResponse::~ReleaseWritelockResponse() {}
}