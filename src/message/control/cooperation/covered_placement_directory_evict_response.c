#include "message/control/cooperation/covered_placement_directory_evict_response.h"

namespace covered
{
    const std::string CoveredPlacementDirectoryEvictResponse::kClassName("CoveredPlacementDirectoryEvictResponse");

    CoveredPlacementDirectoryEvictResponse::CoveredPlacementDirectoryEvictResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteVictimsetEdgesetMessage(key, static_cast<uint8_t>(is_being_written), victim_syncset, edgeset, MessageType::kCoveredPlacementDirectoryEvictResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredPlacementDirectoryEvictResponse::CoveredPlacementDirectoryEvictResponse(const DynamicArray& msg_payload) : KeyByteVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementDirectoryEvictResponse::~CoveredPlacementDirectoryEvictResponse() {}

    bool CoveredPlacementDirectoryEvictResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}