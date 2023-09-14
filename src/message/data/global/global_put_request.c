#include "message/data/global/global_put_request.h"

namespace covered
{
    const std::string GlobalPutRequest::kClassName("GlobalPutRequest");

    GlobalPutRequest::GlobalPutRequest(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueMessage(key, value, MessageType::kGlobalPutRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    GlobalPutRequest::GlobalPutRequest(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    GlobalPutRequest::~GlobalPutRequest() {}
}