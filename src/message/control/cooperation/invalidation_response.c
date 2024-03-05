#include "message/control/cooperation/invalidation_response.h"

namespace covered
{
    const std::string InvalidationResponse::kClassName("InvalidationResponse");

    InvalidationResponse::InvalidationResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kInvalidationResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    InvalidationResponse::InvalidationResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    InvalidationResponse::~InvalidationResponse() {}
}