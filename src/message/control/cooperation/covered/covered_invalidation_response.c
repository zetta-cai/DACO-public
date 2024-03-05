#include "message/control/cooperation/covered/covered_invalidation_response.h"

namespace covered
{
    const std::string CoveredInvalidationResponse::kClassName("CoveredInvalidationResponse");

    // CoveredInvalidationResponse::CoveredInvalidationResponse(const Key& key, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyVictimsetMessage(key, victim_syncset, MessageType::kCoveredInvalidationResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    // {
    // }

    CoveredInvalidationResponse::CoveredInvalidationResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kCoveredInvalidationResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    // CoveredInvalidationResponse::CoveredInvalidationResponse(const DynamicArray& msg_payload) : KeyVictimsetMessage(msg_payload)
    // {
    // }

    CoveredInvalidationResponse::CoveredInvalidationResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    CoveredInvalidationResponse::~CoveredInvalidationResponse() {}
}