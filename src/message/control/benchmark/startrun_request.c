#include "message/control/benchmark/startrun_request.h"

namespace covered
{
    const std::string StartrunRequest::kClassName("StartrunRequest");

    StartrunRequest::StartrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kStartrunRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    StartrunRequest::StartrunRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    StartrunRequest::~StartrunRequest() {}
}