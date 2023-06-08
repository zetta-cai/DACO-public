#include "message/control/directory_update_request.h"

namespace covered
{
    const std::string DirectoryUpdateRequest::kClassName("DirectoryUpdateRequest");

    DirectoryUpdateRequest::DirectoryUpdateRequest(const Key& key, const bool& is_directory_exist, const DirectoryInfo& directory_info) : KeyExistenceDirectoryMessage(key, is_directory_exist, directory_info, MessageType::kDirectoryUpdateRequest)
    {
    }

    DirectoryUpdateRequest::DirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyExistenceDirectoryMessage(msg_payload)
    {
    }

    DirectoryUpdateRequest::~DirectoryUpdateRequest() {}
}