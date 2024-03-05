#include "message/control/cooperation/bestguess/bestguess_bgplace_directory_update_request.h"

namespace covered
{
    const std::string BestGuessBgplaceDirectoryUpdateRequest::kClassName("BestGuessBgplaceDirectoryUpdateRequest");

    BestGuessBgplaceDirectoryUpdateRequest::BestGuessBgplaceDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BestGuessSyncinfo& bestguess_syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValidityDirectorySyncinfoMessage(key, is_valid_directory_exist, directory_info, bestguess_syncinfo, MessageType::kBestGuessBgplaceDirectoryUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessBgplaceDirectoryUpdateRequest::BestGuessBgplaceDirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyValidityDirectorySyncinfoMessage(msg_payload)
    {
    }

    BestGuessBgplaceDirectoryUpdateRequest::~BestGuessBgplaceDirectoryUpdateRequest() {}
}