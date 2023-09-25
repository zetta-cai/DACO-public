#include "message/data/redirected/covered_placement_redirected_get_request.h"

namespace covered
{
    const std::string CoveredPlacementRedirectedGetRequest::kClassName("CoveredPlacementRedirectedGetRequest");

    CoveredPlacementRedirectedGetRequest::CoveredPlacementRedirectedGetRequest(const Key& key, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyVictimsetEdgesetMessage(key, victim_syncset, edgeset, MessageType::kCoveredPlacementRedirectedGetRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredPlacementRedirectedGetRequest::CoveredPlacementRedirectedGetRequest(const DynamicArray& msg_payload) : KeyVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementRedirectedGetRequest::~CoveredPlacementRedirectedGetRequest() {}
}