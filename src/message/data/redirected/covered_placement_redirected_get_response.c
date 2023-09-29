#include "message/data/redirected/covered_placement_redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string CoveredPlacementRedirectedGetResponse::kClassName("CoveredPlacementRedirectedGetResponse");

    CoveredPlacementRedirectedGetResponse::CoveredPlacementRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueHitflagVictimsetEdgesetMessage(key, value, hitflag, victim_syncset, edgeset, MessageType::kCoveredPlacementRedirectedGetResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        // TODO: hitflag may be Hitflag::kCooperativeInvalid in corner cases???
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss);
    }

    CoveredPlacementRedirectedGetResponse::CoveredPlacementRedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementRedirectedGetResponse::~CoveredPlacementRedirectedGetResponse() {}
}