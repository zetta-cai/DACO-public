#include "message/control/cooperation/covered_placement_directory_update_response.h"

namespace covered
{
    const std::string CoveredPlacementDirectoryUpdateResponse::kClassName("CoveredPlacementDirectoryUpdateResponse");

    CoveredPlacementDirectoryUpdateResponse::CoveredPlacementDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), victim_syncset, MessageType::kCoveredPlacementDirectoryUpdateResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementDirectoryUpdateResponse::CoveredPlacementDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteVictimsetMessage(msg_payload)
    {
    }

    CoveredPlacementDirectoryUpdateResponse::~CoveredPlacementDirectoryUpdateResponse() {}

    bool CoveredPlacementDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}