#include "message/control/benchmark/finish_warmup_response.h"

namespace covered
{
    const std::string FinishWarmupResponse::kClassName("FinishWarmupResponse");

    // NOTE: use BandwidthUsage() as we do NOT need to count benchmark control messages for data plane bandwidth usage
    FinishWarmupResponse::FinishWarmupResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kFinishWarmupResponse, source_index, source_addr, BandwidthUsage(), event_list, ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    FinishWarmupResponse::FinishWarmupResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    FinishWarmupResponse::~FinishWarmupResponse() {}
}