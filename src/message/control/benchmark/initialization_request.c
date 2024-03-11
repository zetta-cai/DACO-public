#include "message/control/benchmark/initialization_request.h"

namespace covered
{
    const std::string InitializationRequest::kClassName("InitializationRequest");

    InitializationRequest::InitializationRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kInitializationRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: only msg seqnum of extra common msghdr will be used
    {
    }

    InitializationRequest::InitializationRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    InitializationRequest::~InitializationRequest() {}
}