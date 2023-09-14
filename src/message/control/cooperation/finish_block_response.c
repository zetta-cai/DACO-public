#include "message/control/cooperation/finish_block_response.h"

namespace covered
{
    const std::string FinishBlockResponse::kClassName("FinishBlockResponse");

    FinishBlockResponse::FinishBlockResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kFinishBlockResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    FinishBlockResponse::FinishBlockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockResponse::~FinishBlockResponse() {}
}