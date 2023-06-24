#include "message/control/directory_lookup_request.h"

namespace covered
{
    const std::string DirectoryLookupRequest::kClassName("DirectoryLookupRequest");

    DirectoryLookupRequest::DirectoryLookupRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kDirectoryLookupRequest, source_index)
    {
    }

    DirectoryLookupRequest::DirectoryLookupRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    DirectoryLookupRequest::~DirectoryLookupRequest() {}
}