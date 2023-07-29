#include "message/control/benchmark/finish_warmup_response.h"

namespace covered
{
    const std::string FinishWarmupResponse::kClassName("FinishWarmupResponse");

    FinishWarmupResponse::FinishWarmupResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kFinishWarmupResponse, source_index, source_addr, event_list, true)
    {
    }

    FinishWarmupResponse::FinishWarmupResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    FinishWarmupResponse::~FinishWarmupResponse() {}
}