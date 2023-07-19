#include "message/control/release_writelock_response.h"

namespace covered
{
    const std::string ReleaseWritelockResponse::kClassName("ReleaseWritelockResponse");

    ReleaseWritelockResponse::ReleaseWritelockResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyMessage(key, MessageType::kReleaseWritelockResponse, source_index, source_addr, event_list)
    {
    }

    ReleaseWritelockResponse::ReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockResponse::~ReleaseWritelockResponse() {}
}