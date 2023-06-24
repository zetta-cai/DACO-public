#include "message/control/directory_update_request.h"

namespace covered
{
    const std::string DirectoryUpdateRequest::kClassName("DirectoryUpdateRequest");

    DirectoryUpdateRequest::DirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index) : KeyValidityDirectoryMessage(key, is_valid_directory_exist, directory_info, MessageType::kDirectoryUpdateRequest, source_index)
    {
    }

    DirectoryUpdateRequest::DirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyValidityDirectoryMessage(msg_payload)
    {
    }

    DirectoryUpdateRequest::~DirectoryUpdateRequest() {}
}