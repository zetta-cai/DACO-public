#include "message/control/cooperation/covered/covered_fghybrid_directory_evict_response.h"

namespace covered
{
    const std::string CoveredFghybridDirectoryEvictResponse::kClassName("CoveredFghybridDirectoryEvictResponse");

    CoveredFghybridDirectoryEvictResponse::CoveredFghybridDirectoryEvictResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteVictimsetEdgesetMessage(key, static_cast<uint8_t>(is_being_written), victim_syncset, edgeset, MessageType::kCoveredFghybridDirectoryEvictResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredFghybridDirectoryEvictResponse::CoveredFghybridDirectoryEvictResponse(const DynamicArray& msg_payload) : KeyByteVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridDirectoryEvictResponse::~CoveredFghybridDirectoryEvictResponse() {}

    bool CoveredFghybridDirectoryEvictResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}