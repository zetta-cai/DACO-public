#include "message/control/cooperation/directory_update_response.h"

namespace covered
{
    const std::string DirectoryUpdateResponse::kClassName("DirectoryUpdateResponse");

    DirectoryUpdateResponse::DirectoryUpdateResponse(const Key& key, const bool& is_being_written, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyByteMessage(key, static_cast<uint8_t>(is_being_written), MessageType::kDirectoryUpdateResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    DirectoryUpdateResponse::DirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    DirectoryUpdateResponse::~DirectoryUpdateResponse() {}

    bool DirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}