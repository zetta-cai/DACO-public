#include "message/control/cooperation/bestguess/bestguess_bgplace_directory_update_response.h"

namespace covered
{
    const std::string BestGuessBgplaceDirectoryUpdateResponse::kClassName("BestGuessBgplaceDirectoryUpdateResponse");

    BestGuessBgplaceDirectoryUpdateResponse::BestGuessBgplaceDirectoryUpdateResponse(const Key& key, const bool& is_being_written, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyByteSyncinfoMessage(key, static_cast<uint8_t>(is_being_written), syncinfo, MessageType::kBestGuessBgplaceDirectoryUpdateResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessBgplaceDirectoryUpdateResponse::BestGuessBgplaceDirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessBgplaceDirectoryUpdateResponse::~BestGuessBgplaceDirectoryUpdateResponse() {}

    bool BestGuessBgplaceDirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}