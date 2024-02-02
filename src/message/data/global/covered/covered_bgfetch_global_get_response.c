#include "message/data/global/covered/covered_bgfetch_global_get_response.h"

namespace covered
{
    const std::string CoveredBgfetchGlobalGetResponse::kClassName("CoveredBgfetchGlobalGetResponse");

    CoveredBgfetchGlobalGetResponse::CoveredBgfetchGlobalGetResponse(const Key& key, const Value& value, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueEdgesetMessage(key, value, edgeset, MessageType::kCoveredBgfetchGlobalGetResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredBgfetchGlobalGetResponse::CoveredBgfetchGlobalGetResponse(const DynamicArray& msg_payload) : KeyValueEdgesetMessage(msg_payload)
    {
    }

    CoveredBgfetchGlobalGetResponse::~CoveredBgfetchGlobalGetResponse() {}
}