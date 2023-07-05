#include "message/control/invalidation_response.h"

namespace covered
{
    const std::string InvalidationResponse::kClassName("InvalidationResponse");

    InvalidationResponse::InvalidationResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kInvalidationResponse, source_index, source_addr)
    {
    }

    InvalidationResponse::InvalidationResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    InvalidationResponse::~InvalidationResponse() {}
}