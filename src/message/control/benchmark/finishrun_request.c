#include "message/control/benchmark/finishrun_request.h"

namespace covered
{
    const std::string FinishrunRequest::kClassName("FinishrunRequest");

    FinishrunRequest::FinishrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr) : SimpleMessage(MessageType::kFinishrunRequest, source_index, source_addr, EventList())
    {
    }

    FinishrunRequest::FinishrunRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    FinishrunRequest::~FinishrunRequest() {}
}