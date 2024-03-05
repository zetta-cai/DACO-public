#include "message/control/cooperation/release_writelock_response.h"

namespace covered
{
    const std::string ReleaseWritelockResponse::kClassName("ReleaseWritelockResponse");

    ReleaseWritelockResponse::ReleaseWritelockResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kReleaseWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    ReleaseWritelockResponse::ReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    ReleaseWritelockResponse::~ReleaseWritelockResponse() {}
}