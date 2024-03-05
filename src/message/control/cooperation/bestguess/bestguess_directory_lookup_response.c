#include "message/control/cooperation/bestguess/bestguess_directory_lookup_response.h"

namespace covered
{
    const std::string BestGuessDirectoryLookupResponse::kClassName("BestGuessDirectoryLookupResponse");

    BestGuessDirectoryLookupResponse::BestGuessDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyWriteflagValidityDirectorySyncinfoMessage(key, is_being_written, is_valid_directory_exist, directory_info, syncinfo, MessageType::kBestGuessDirectoryLookupResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessDirectoryLookupResponse::BestGuessDirectoryLookupResponse(const DynamicArray& msg_payload) : KeyWriteflagValidityDirectorySyncinfoMessage(msg_payload)
    {
    }

    BestGuessDirectoryLookupResponse::~BestGuessDirectoryLookupResponse() {}
}