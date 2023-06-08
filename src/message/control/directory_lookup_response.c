#include "message/control/directory_lookup_response.h"

namespace covered
{
    const std::string DirectoryLookupResponse::kClassName("DirectoryLookupResponse");

    DirectoryLookupResponse::DirectoryLookupResponse(const Key& key, const bool& is_directory_exist, const DirectoryInfo& directory_info) : KeyExistenceDirectoryMessage(key, is_directory_exist, directory_info, MessageType::kDirectoryLookupResponse)
    {
    }

    DirectoryLookupResponse::DirectoryLookupResponse(const DynamicArray& msg_payload) : KeyExistenceDirectoryMessage(msg_payload)
    {
    }

    DirectoryLookupResponse::~DirectoryLookupResponse() {}
}