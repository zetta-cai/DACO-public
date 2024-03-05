#include "message/control/cooperation/covered/covered_placement_trigger_request.h"

namespace covered
{
    const std::string CoveredPlacementTriggerRequest::kClassName("CoveredPlacementTriggerRequest");

    CoveredPlacementTriggerRequest::CoveredPlacementTriggerRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyCollectpopVictimsetMessage(key, collected_popularity, victim_syncset, MessageType::kCoveredPlacementTriggerRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    CoveredPlacementTriggerRequest::CoveredPlacementTriggerRequest(const DynamicArray& msg_payload) : KeyCollectpopVictimsetMessage(msg_payload)
    {
    }

    CoveredPlacementTriggerRequest::~CoveredPlacementTriggerRequest() {}
}