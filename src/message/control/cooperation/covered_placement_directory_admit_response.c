#include "message/control/cooperation/covered_placement_directory_admit_response.h"

namespace covered
{
    const std::string CoveredPlacementDirectoryAdmitResponse::kClassName = "CoveredPlacementDirectoryAdmitResponse";

    CoveredPlacementDirectoryAdmitResponse::CoveredPlacementDirectoryAdmitResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyTwoByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), static_cast<uint8_t>(is_neighbor_cached), victim_syncset, MessageType::kCoveredPlacementDirectoryAdmitResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementDirectoryAdmitResponse::CoveredPlacementDirectoryAdmitResponse(const DynamicArray& msg_payload) : KeyTwoByteVictimsetMessage(msg_payload)
    {
    }

    CoveredPlacementDirectoryAdmitResponse::~CoveredPlacementDirectoryAdmitResponse()
    {
    }

    bool CoveredPlacementDirectoryAdmitResponse::isBeingWritten() const
    {
        return static_cast<bool>(this->getFirstByte_());
    }

    bool CoveredPlacementDirectoryAdmitResponse::isNeighborCached() const
    {
        return static_cast<bool>(this->getSecondByte_());
    }
}