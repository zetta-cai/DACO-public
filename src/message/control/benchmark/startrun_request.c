#include "message/control/benchmark/startrun_request.h"

namespace covered
{
    const std::string StartrunRequest::kClassName("StartrunRequest");

    StartrunRequest::StartrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr) : SimpleMessage(MessageType::kStartrunRequest, source_index, source_addr, EventList(), true)
    {
    }

    StartrunRequest::StartrunRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    StartrunRequest::~StartrunRequest() {}
}