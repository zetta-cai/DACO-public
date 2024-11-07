#include "message/control/benchmark/update_rules_response.h"

namespace covered
{
    const std::string UpdateRulesResponse::kClassName("UpdateRulesResponse");

    // NOTE: use BandwidthUsage() as we do NOT need to count benchmark control messages for data plane bandwidth usage
    UpdateRulesResponse::UpdateRulesResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kUpdateRulesResponse, source_index, source_addr, BandwidthUsage(), event_list, ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    UpdateRulesResponse::UpdateRulesResponse(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    UpdateRulesResponse::~UpdateRulesResponse() {}
}