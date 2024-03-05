#include "message/control/cooperation/covered/covered_bgplace_directory_update_response.h"

namespace covered
{
    const std::string CoveredBgplaceDirectoryUpdateResponse::kClassName("CoveredBgplaceDirectoryUpdateResponse");

    CoveredBgplaceDirectoryUpdateResponse::CoveredBgplaceDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyTwoByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), static_cast<uint8_t>(is_neighbor_cached), victim_syncset, MessageType::kCoveredBgplaceDirectoryUpdateResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    CoveredBgplaceDirectoryUpdateResponse::CoveredBgplaceDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyTwoByteVictimsetMessage(msg_payload)
    {
    }

    CoveredBgplaceDirectoryUpdateResponse::~CoveredBgplaceDirectoryUpdateResponse() {}

    bool CoveredBgplaceDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getFirstByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }

    bool CoveredBgplaceDirectoryUpdateResponse::isNeighborCached() const
    {
        uint8_t byte = getSecondByte_();
        bool is_neighbor_cached = static_cast<bool>(byte);
        return is_neighbor_cached;
    }
}