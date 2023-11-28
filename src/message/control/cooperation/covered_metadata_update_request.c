#include "message/control/cooperation/covered_metadata_update_request.h"

namespace covered
{
    const std::string CoveredMetadataUpdateRequest::kClassName("CoveredMetadataUpdateRequest");

    CoveredMetadataUpdateRequest::CoveredMetadataUpdateRequest(const Key& key, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyByteVictimsetMessage(key, static_cast<uint8_t>(is_neighbor_cached), victim_syncset, MessageType::kCoveredMetadataUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredMetadataUpdateRequest::CoveredMetadataUpdateRequest(const DynamicArray& msg_payload) : KeyByteVictimsetMessage(msg_payload)
    {
    }

    CoveredMetadataUpdateRequest::~CoveredMetadataUpdateRequest() {}

    bool CoveredMetadataUpdateRequest::isNeighborCached() const
    {
        uint8_t byte = getByte_();
        bool is_neighbor_cached = static_cast<bool>(byte);
        return is_neighbor_cached;
    }
}