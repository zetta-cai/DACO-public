#include "message/control/cooperation/directory_update_request.h"

namespace covered
{
    const std::string DirectoryUpdateRequest::kClassName("DirectoryUpdateRequest");

    DirectoryUpdateRequest::DirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValidityDirectoryMessage(key, is_valid_directory_exist, directory_info, MessageType::kDirectoryUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    DirectoryUpdateRequest::DirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyValidityDirectoryMessage(msg_payload)
    {
    }

    DirectoryUpdateRequest::~DirectoryUpdateRequest() {}
}