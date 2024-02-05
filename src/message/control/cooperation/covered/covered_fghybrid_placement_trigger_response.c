#include "message/control/cooperation/covered/covered_fghybrid_placement_trigger_response.h"

namespace covered
{
    const std::string CoveredFghybridPlacementTriggerResponse::kClassName("CoveredFghybridPlacementTriggerResponse");

    CoveredFghybridPlacementTriggerResponse::CoveredFghybridPlacementTriggerResponse(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyVictimsetEdgesetMessage(key, victim_syncset, edgeset, MessageType::kCoveredFghybridPlacementTriggerResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredFghybridPlacementTriggerResponse::CoveredFghybridPlacementTriggerResponse(const DynamicArray& msg_payload) : KeyVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridPlacementTriggerResponse::~CoveredFghybridPlacementTriggerResponse() {}
}