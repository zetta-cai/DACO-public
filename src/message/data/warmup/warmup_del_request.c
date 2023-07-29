#include "message/data/warmup/warmup_del_request.h"

namespace covered
{
    const std::string WarmupDelRequest::kClassName("WarmupDelRequest");

    WarmupDelRequest::WarmupDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kWarmupDelRequest, source_index, source_addr, EventList())
    {
    }

    WarmupDelRequest::WarmupDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    WarmupDelRequest::~WarmupDelRequest() {}
}