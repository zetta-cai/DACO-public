#include "message/control/benchmark/initialization_response.h"

namespace covered
{
    const std::string InitializationResponse::kClassName("InitializationResponse");

    InitializationResponse::InitializationResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kInitializationResponse, source_index, source_addr, event_list, true)
    {
    }

    InitializationResponse::InitializationResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    InitializationResponse::~InitializationResponse() {}
}