#include "message/data/global/global_put_response.h"

namespace covered
{
    const std::string GlobalPutResponse::kClassName("GlobalPutResponse");

    GlobalPutResponse::GlobalPutResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kGlobalPutResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    GlobalPutResponse::GlobalPutResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalPutResponse::~GlobalPutResponse() {}
}