#include "message/control/cooperation/covered/covered_invalidation_request.h"

namespace covered
{
    const std::string CoveredInvalidationRequest::kClassName("CoveredInvalidationRequest");

    // CoveredInvalidationRequest::CoveredInvalidationRequest(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    // {
    // }

    CoveredInvalidationRequest::CoveredInvalidationRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kCoveredInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
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