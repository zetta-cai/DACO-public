#include "message/data/global/covered_placement_global_get_request.h"

namespace covered
{
    const std::string CoveredPlacementGlobalGetRequest::kClassName("CoveredPlacementGlobalGetRequest");

    CoveredPlacementGlobalGetRequest::CoveredPlacementGlobalGetRequest(const Key& key, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyEdgesetMessage(key, edgeset, MessageType::kCoveredPlacementGlobalGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredPlacementGlobalGetRequest::CoveredPlacementGlobalGetRequest(const DynamicArray& msg_payload) : KeyEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementGlobalGetRequest::~CoveredPlacementGlobalGetRequest() {}
}