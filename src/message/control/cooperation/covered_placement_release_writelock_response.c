#include "message/control/cooperation/covered_placement_release_writelock_response.h"

namespace covered
{
    const std::string CoveredPlacementReleaseWritelockResponse::kClassName("CoveredPlacementReleaseWritelockResponse");

    CoveredPlacementReleaseWritelockResponse::CoveredPlacementReleaseWritelockResponse(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyVictimsetEdgesetMessage(key, victim_syncset, edgeset, MessageType::kCoveredPlacementReleaseWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementReleaseWritelockResponse::CoveredPlacementReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementReleaseWritelockResponse::~CoveredPlacementReleaseWritelockResponse() {}
}