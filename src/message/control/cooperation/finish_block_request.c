#include "message/control/cooperation/finish_block_request.h"

namespace covered
{
    const std::string FinishBlockRequest::kClassName("FinishBlockRequest");

    FinishBlockRequest::FinishBlockRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kFinishBlockRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    FinishBlockRequest::FinishBlockRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockRequest::~FinishBlockRequest() {}
}