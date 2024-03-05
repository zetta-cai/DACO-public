#include "message/control/cooperation/covered/covered_acquire_writelock_request.h"

namespace covered
{
    const std::string CoveredAcquireWritelockRequest::kClassName("CoveredAcquireWritelockRequest");

    CoveredAcquireWritelockRequest::CoveredAcquireWritelockRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyCollectpopVictimsetMessage(key, collected_popularity, victim_syncset, MessageType::kCoveredAcquireWritelockRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    CoveredAcquireWritelockRequest::CoveredAcquireWritelockRequest(const DynamicArray& msg_payload) : KeyCollectpopVictimsetMessage(msg_payload)
    {
    }

    CoveredAcquireWritelockRequest::~CoveredAcquireWritelockRequest() {}
}