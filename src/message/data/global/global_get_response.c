#include "message/data/global/global_get_response.h"

namespace covered
{
    const std::string GlobalGetResponse::kClassName("GlobalGetResponse");

    GlobalGetResponse::GlobalGetResponse(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueMessage(key, value, MessageType::kGlobalGetResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    GlobalGetResponse::GlobalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    GlobalGetResponse::~GlobalGetResponse() {}
}