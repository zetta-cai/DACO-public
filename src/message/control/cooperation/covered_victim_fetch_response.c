#include "message/control/cooperation/covered_victim_fetch_response.h"

namespace covered
{
    const std::string CoveredVictimFetchResponse::kClassName("CoveredVictimFetchResponse");

    // CoveredVictimFetchResponse::CoveredVictimFetchResponse(const VictimSyncset& victim_fetchset, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : DoubleVictimsetMessage(victim_fetchset, victim_syncset, MessageType::kCoveredVictimFetchResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    // {
    // }

    CoveredVictimFetchResponse::CoveredVictimFetchResponse(const VictimSyncset& victim_fetchset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : SingleVictimsetMessage(victim_fetchset, MessageType::kCoveredVictimFetchResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    // CoveredVictimFetchResponse::CoveredVictimFetchResponse(const DynamicArray& msg_payload) : DoubleVictimsetMessage(msg_payload)
    // {
    // }

    CoveredVictimFetchResponse::CoveredVictimFetchResponse(const DynamicArray& msg_payload) : SingleVictimsetMessage(msg_payload)
    {
    }

    CoveredVictimFetchResponse::~CoveredVictimFetchResponse() {}
}