#include "message/control/cooperation/covered/covered_fghybrid_directory_admit_response.h"

namespace covered
{
    const std::string CoveredFghybridDirectoryAdmitResponse::kClassName = "CoveredFghybridDirectoryAdmitResponse";

    CoveredFghybridDirectoryAdmitResponse::CoveredFghybridDirectoryAdmitResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyTwoByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), static_cast<uint8_t>(is_neighbor_cached), victim_syncset, MessageType::kCoveredFghybridDirectoryAdmitResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredFghybridDirectoryAdmitResponse::CoveredFghybridDirectoryAdmitResponse(const DynamicArray& msg_payload) : KeyTwoByteVictimsetMessage(msg_payload)
    {
    }

    CoveredFghybridDirectoryAdmitResponse::~CoveredFghybridDirectoryAdmitResponse()
    {
    }

    bool CoveredFghybridDirectoryAdmitResponse::isBeingWritten() const
    {
        return static_cast<bool>(this->getFirstByte_());
    }

    bool CoveredFghybridDirectoryAdmitResponse::isNeighborCached() const
    {
        return static_cast<bool>(this->getSecondByte_());
    }
}