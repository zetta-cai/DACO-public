#include "message/control/cooperation/bestguess/bestguess_directory_update_response.h"

namespace covered
{
    const std::string BestGuessDirectoryUpdateResponse::kClassName("BestGuessDirectoryUpdateResponse");

    BestGuessDirectoryUpdateResponse::BestGuessDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteSyncinfoMessage(key, static_cast<uint8_t>(is_being_written), syncinfo, MessageType::kBestGuessDirectoryUpdateResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    BestGuessDirectoryUpdateResponse::BestGuessDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessDirectoryUpdateResponse::~BestGuessDirectoryUpdateResponse() {}

    bool BestGuessDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}