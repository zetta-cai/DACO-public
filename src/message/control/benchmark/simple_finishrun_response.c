#include "message/control/benchmark/simple_finishrun_response.h"

namespace covered
{
    const std::string SimpleFinishrunResponse::kClassName("SimpleFinishrunResponse");

    SimpleFinishrunResponse::SimpleFinishrunResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kSimpleFinishrunResponse, source_index, source_addr, event_list, true)
    {
    }

    SimpleFinishrunResponse::SimpleFinishrunResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    SimpleFinishrunResponse::~SimpleFinishrunResponse() {}
}