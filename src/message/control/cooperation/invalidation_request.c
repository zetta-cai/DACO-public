#include "message/control/cooperation/invalidation_request.h"

namespace covered
{
    const std::string InvalidationRequest::kClassName("InvalidationRequest");

    InvalidationRequest::InvalidationRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    InvalidationRequest::InvalidationRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    InvalidationRequest::~InvalidationRequest() {}
}