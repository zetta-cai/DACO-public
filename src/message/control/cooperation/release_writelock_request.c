#include "message/control/cooperation/release_writelock_request.h"

namespace covered
{
    const std::string ReleaseWritelockRequest::kClassName("ReleaseWritelockRequest");

    ReleaseWritelockRequest::ReleaseWritelockRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kReleaseWritelockRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    ReleaseWritelockRequest::ReleaseWritelockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockRequest::~ReleaseWritelockRequest() {}
}