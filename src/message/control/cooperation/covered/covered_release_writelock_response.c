#include "message/control/cooperation/covered/covered_release_writelock_response.h"

namespace covered
{
    const std::string CoveredReleaseWritelockResponse::kClassName("CoveredReleaseWritelockResponse");

    CoveredReleaseWritelockResponse::CoveredReleaseWritelockResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredReleaseWritelockResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredReleaseWritelockResponse::CoveredReleaseWritelockResponse(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    {
    }

    CoveredReleaseWritelockResponse::~CoveredReleaseWritelockResponse() {}
}