#include "message/control/benchmark/startrun_response.h"

namespace covered
{
    const std::string StartrunResponse::kClassName("StartrunResponse");

    StartrunResponse::StartrunResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kStartrunResponse, source_index, source_addr, event_list)
    {
    }

    StartrunResponse::StartrunResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    StartrunResponse::~StartrunResponse() {}
}