#include "message/control/benchmark/initialization_response.h"

namespace covered
{
    const std::string InitializationResponse::kClassName("InitializationResponse");

    // NOTE: use BandwidthUsage() as we do NOT need to count benchmark control messages for data plane bandwidth usage
    InitializationResponse::InitializationResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : SimpleMessage(MessageType::kInitializationResponse, source_index, source_addr, BandwidthUsage(), event_list, ExtraCommonMsghdr())
    {
    }

    InitializationResponse::InitializationResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    InitializationResponse::~InitializationResponse() {}
}