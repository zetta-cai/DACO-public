#include "message/control/cooperation/covered_placement_hybrid_fetched_request.h"

namespace covered
{
    const std::string CoveredPlacementHybridFetchedRequest::kClassName = "CoveredPlacementHybridFetchedRequest";

    CoveredPlacementHybridFetchedRequest::CoveredPlacementHybridFetchedRequest(const Key& key, const Value& value, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueVictimsetEdgesetMessage(key, value, victim_syncset, edgeset, MessageType::kCoveredPlacementHybridFetchedRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredPlacementHybridFetchedRequest::CoveredPlacementHybridFetchedRequest(const DynamicArray& msg_payload) : KeyValueVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementHybridFetchedRequest::~CoveredPlacementHybridFetchedRequest()
    {
    }
}