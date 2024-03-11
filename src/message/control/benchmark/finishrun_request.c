#include "message/control/benchmark/finishrun_request.h"

namespace covered
{
    const std::string FinishrunRequest::kClassName("FinishrunRequest");

    FinishrunRequest::FinishrunRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kFinishrunRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    FinishrunRequest::FinishrunRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    FinishrunRequest::~FinishrunRequest() {}
}