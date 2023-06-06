#include "message/control/directory_lookup_response.h"

namespace covered
{
    const std::string DirectoryLookupResponse::kClassName("DirectoryLookupResponse");

    DirectoryLookupResponse::DirectoryLookupResponse(const Key& key, const uint32_t& neighbor_edge_idx) : KeyDirectoryMessage(key, neighbor_edge_idx, MessageType::kDirectoryLookupResponse)
    {
    }

    DirectoryLookupResponse::DirectoryLookupResponse(const DynamicArray& msg_payload) : KeyDirectoryMessage(msg_payload)
    {
    }

    DirectoryLookupResponse::~DirectoryLookupResponse() {}
}