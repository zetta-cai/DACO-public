#include "message/data/global/global_del_response.h"

namespace covered
{
    const std::string GlobalDelResponse::kClassName("GlobalDelResponse");

    GlobalDelResponse::GlobalDelResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kGlobalDelResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    GlobalDelResponse::GlobalDelResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelResponse::~GlobalDelResponse() {}
}