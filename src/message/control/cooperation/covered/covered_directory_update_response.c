#include "message/control/cooperation/covered/covered_directory_update_response.h"

namespace covered
{
    const std::string CoveredDirectoryUpdateResponse::kClassName("CoveredDirectoryUpdateResponse");

    CoveredDirectoryUpdateResponse::CoveredDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyTwoByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), static_cast<uint8_t>(is_neighbor_cached), victim_syncset, MessageType::kCoveredDirectoryUpdateResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredDirectoryUpdateResponse::CoveredDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyTwoByteVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryUpdateResponse::~CoveredDirectoryUpdateResponse() {}

    bool CoveredDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getFirstByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }

    bool CoveredDirectoryUpdateResponse::isNeighborCached() const
    {
        uint8_t byte = getSecondByte_();
        bool is_neighbor_cached = static_cast<bool>(byte);
        return is_neighbor_cached;
    }
}