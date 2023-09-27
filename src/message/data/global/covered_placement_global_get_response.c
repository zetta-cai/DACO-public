#include "message/data/global/covered_placement_global_get_response.h"

namespace covered
{
    const std::string CoveredPlacementGlobalGetResponse::kClassName("CoveredPlacementGlobalGetResponse");

    CoveredPlacementGlobalGetResponse::CoveredPlacementGlobalGetResponse(const Key& key, const Value& value, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueEdgesetMessage(key, value, edgeset, MessageType::kCoveredPlacementGlobalGetResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementGlobalGetResponse::CoveredPlacementGlobalGetResponse(const DynamicArray& msg_payload) : KeyValueEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementGlobalGetResponse::~CoveredPlacementGlobalGetResponse() {}
}