#include "message/control/cooperation/bestguess/bestguess_directory_admit_response.h"

namespace covered
{
    const std::string BestGuessDirectoryAdmitResponse::kClassName("BestGuessDirectoryAdmitResponse");

    BestGuessDirectoryAdmitResponse::BestGuessDirectoryAdmitResponse(const Key& key, const bool& is_being_written, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteSyncinfoMessage(key, static_cast<uint8_t>(is_being_written), syncinfo, MessageType::kBestGuessDirectoryAdmitResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    BestGuessDirectoryAdmitResponse::BestGuessDirectoryAdmitResponse(const DynamicArray& msg_payload) : KeyByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessDirectoryAdmitResponse::~BestGuessDirectoryAdmitResponse() {}

    bool BestGuessDirectoryAdmitResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}