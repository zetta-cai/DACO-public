#include "message/control/cooperation/covered_release_writelock_request.h"

namespace covered
{
    const std::string CoveredReleaseWritelockRequest::kClassName("CoveredReleaseWritelockRequest");

    CoveredReleaseWritelockRequest::CoveredReleaseWritelockRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyCollectpopVictimsetMessage(key, collected_popularity, victim_syncset, MessageType::kCoveredReleaseWritelockRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredReleaseWritelockRequest::CoveredReleaseWritelockRequest(const DynamicArray& msg_payload) : KeyCollectpopVictimsetMessage(msg_payload)
    {
    }

    CoveredReleaseWritelockRequest::~CoveredReleaseWritelockRequest() {}
}