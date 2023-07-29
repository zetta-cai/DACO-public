#include "message/data/global/global_del_request.h"

namespace covered
{
    const std::string GlobalDelRequest::kClassName("GlobalDelRequest");

    GlobalDelRequest::GlobalDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kGlobalDelRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    GlobalDelRequest::GlobalDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelRequest::~GlobalDelRequest() {}
}