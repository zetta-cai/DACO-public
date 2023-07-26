#include "message/control/cooperation/invalidation_response.h"

namespace covered
{
    const std::string InvalidationResponse::kClassName("InvalidationResponse");

    InvalidationResponse::InvalidationResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyMessage(key, MessageType::kInvalidationResponse, source_index, source_addr, event_list)
    {
    }

    InvalidationResponse::InvalidationResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    InvalidationResponse::~InvalidationResponse() {}
}