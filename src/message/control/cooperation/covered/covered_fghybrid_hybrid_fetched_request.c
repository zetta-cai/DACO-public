#include "message/control/cooperation/covered/covered_fghybrid_hybrid_fetched_request.h"

namespace covered
{
    const std::string CoveredFghybridHybridFetchedRequest::kClassName = "CoveredFghybridHybridFetchedRequest";

    CoveredFghybridHybridFetchedRequest::CoveredFghybridHybridFetchedRequest(const Key& key, const Value& value, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueVictimsetEdgesetMessage(key, value, victim_syncset, edgeset, MessageType::kCoveredPlacementHybridFetchedRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredFghybridHybridFetchedRequest::CoveredFghybridHybridFetchedRequest(const DynamicArray& msg_payload) : KeyValueVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridHybridFetchedRequest::~CoveredFghybridHybridFetchedRequest()
    {
    }
}