#include "message/control/benchmark/finish_warmup_request.h"

namespace covered
{
    const std::string FinishWarmupRequest::kClassName("FinishWarmupRequest");

    FinishWarmupRequest::FinishWarmupRequest(const uint32_t& source_index, const NetworkAddr& source_addr) : SimpleMessage(MessageType::kFinishWarmupRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr())
    {
    }

    FinishWarmupRequest::FinishWarmupRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    FinishWarmupRequest::~FinishWarmupRequest() {}
}