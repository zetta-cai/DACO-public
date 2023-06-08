#include "message/control/directory_lookup_response.h"

namespace covered
{
    const std::string DirectoryLookupResponse::kClassName("DirectoryLookupResponse");

    DirectoryLookupResponse::DirectoryLookupResponse(const Key& key, const bool& is_directory_exist, const uint32_t& target_edge_idx) : KeyDirectoryMessage(key, is_directory_exist, target_edge_idx, MessageType::kDirectoryLookupResponse)
    {
    }

    DirectoryLookupResponse::DirectoryLookupResponse(const DynamicArray& msg_payload) : KeyDirectoryMessage(msg_payload)
    {
    }

    DirectoryLookupResponse::~DirectoryLookupResponse() {}
}