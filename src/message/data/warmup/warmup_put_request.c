#include "message/data/warmup/warmup_put_request.h"

namespace covered
{
    const std::string WarmupPutRequest::kClassName("WarmupPutRequest");

    WarmupPutRequest::WarmupPutRequest(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyValueMessage(key, value, MessageType::kWarmupPutRequest, source_index, source_addr, EventList())
    {
    }

    WarmupPutRequest::WarmupPutRequest(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    WarmupPutRequest::~WarmupPutRequest() {}
}