#include "message/control/cooperation/covered_directory_update_response.h"

namespace covered
{
    const std::string CoveredDirectoryUpdateResponse::kClassName("CoveredDirectoryUpdateResponse");

    CoveredDirectoryUpdateResponse::CoveredDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteVictimsetMessage(key, static_cast<uint8_t>(is_being_written), victim_syncset, MessageType::kCoveredDirectoryUpdateResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    CoveredDirectoryUpdateResponse::CoveredDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryUpdateResponse::~CoveredDirectoryUpdateResponse() {}

    bool CoveredDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}