#include "message/control/benchmark/simple_finishrun_response.h"

namespace covered
{
    const std::string SimpleFinishrunResponse::kClassName("SimpleFinishrunResponse");

    // NOTE: use BandwidthUsage() as we do NOT need to count benchmark control messages for data plane bandwidth usage
    SimpleFinishrunResponse::SimpleFinishrunResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kSimpleFinishrunResponse, source_index, source_addr, BandwidthUsage(), event_list, ExtraCommonMsghdr())
    {
    }

    SimpleFinishrunResponse::SimpleFinishrunResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    SimpleFinishrunResponse::~SimpleFinishrunResponse() {}
}