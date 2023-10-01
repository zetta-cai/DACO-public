#include "message/data/management/covered_placement_notify_request.h"

#include <assert.h>

namespace covered
{
    const std::string CoveredPlacementNotifyRequest::kClassName("CoveredPlacementNotifyRequest");

    CoveredPlacementNotifyRequest::CoveredPlacementNotifyRequest(const Key& key, const Value& value, const bool& is_valid, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueByteVictimsetMessage(key, value, static_cast<uint8_t>(is_valid), victim_syncset, MessageType::kCoveredPlacementNotifyRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredPlacementNotifyRequest::CoveredPlacementNotifyRequest(const DynamicArray& msg_payload) : KeyValueByteVictimsetMessage(msg_payload)
    {
    }

    CoveredPlacementNotifyRequest::~CoveredPlacementNotifyRequest() {}

    bool CoveredPlacementNotifyRequest::isValid() const
    {
        return static_cast<bool>(getByte_());
    }
}