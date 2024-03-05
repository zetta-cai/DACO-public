#include "message/control/cooperation/bestguess/bestguess_directory_update_request.h"

namespace covered
{
    const std::string BestGuessDirectoryUpdateRequest::kClassName("BestGuessDirectoryUpdateRequest");

    BestGuessDirectoryUpdateRequest::BestGuessDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& bestguess_syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValidityDirectorySyncinfoMessage(key, is_valid_directory_exist, directory_info, bestguess_syncinfo, MessageType::kBestGuessDirectoryUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessDirectoryUpdateRequest::BestGuessDirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyValidityDirectorySyncinfoMessage(msg_payload)
    {
    }

    BestGuessDirectoryUpdateRequest::~BestGuessDirectoryUpdateRequest() {}
}