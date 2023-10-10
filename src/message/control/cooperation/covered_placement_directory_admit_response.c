#include "message/control/cooperation/covered_placement_directory_admit_response.h"

namespace covered
{
    const std::string CoveredPlacementDirectoryAdmitResponse::kClassName = "CoveredPlacementDirectoryAdmitResponse";

    CoveredPlacementDirectoryAdmitResponse::CoveredPlacementDirectoryAdmitResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), victim_syncset, MessageType::kCoveredPlacementDirectoryAdmitResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementDirectoryAdmitResponse::CoveredPlacementDirectoryAdmitResponse(const DynamicArray& msg_payload) : KeyByteVictimsetMessage(msg_payload)
    {
    }

    CoveredPlacementDirectoryAdmitResponse::~CoveredPlacementDirectoryAdmitResponse()
    {
    }

    bool CoveredPlacementDirectoryAdmitResponse::isBeingWritten() const
    {
        return static_cast<bool>(this->getByte());
    }
}