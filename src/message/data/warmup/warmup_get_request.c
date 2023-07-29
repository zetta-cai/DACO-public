#include "message/data/warmup/warmup_get_request.h"

namespace covered
{
    const std::string WarmupGetRequest::kClassName("WarmupGetRequest");

    WarmupGetRequest::WarmupGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kWarmupGetRequest, source_index, source_addr, EventList())
    {
    }

    WarmupGetRequest::WarmupGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    WarmupGetRequest::~WarmupGetRequest() {}
}