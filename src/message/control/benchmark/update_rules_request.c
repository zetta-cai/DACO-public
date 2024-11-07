#include "message/control/benchmark/update_rules_request.h"

namespace covered
{
    const std::string UpdateRulesRequest::kClassName("UpdateRulesRequest");

    UpdateRulesRequest::UpdateRulesRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kUpdateRulesRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    UpdateRulesRequest::UpdateRulesRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    UpdateRulesRequest::~UpdateRulesRequest() {}
}