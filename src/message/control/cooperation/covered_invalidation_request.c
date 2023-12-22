#include "message/control/cooperation/covered_invalidation_request.h"

namespace covered
{
    const std::string CoveredInvalidationRequest::kClassName("CoveredInvalidationRequest");

    // CoveredInvalidationRequest::CoveredInvalidationRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    // {
    // }

    CoveredInvalidationRequest::CoveredInvalidationRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kCoveredInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    // CoveredInvalidationRequest::CoveredInvalidationRequest(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    // {
    // }

    CoveredInvalidationRequest::CoveredInvalidationRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    CoveredInvalidationRequest::~CoveredInvalidationRequest() {}
}