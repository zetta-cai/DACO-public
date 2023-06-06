#include "message/control/directory_lookup_request.h"

namespace covered
{
    const std::string DirectoryLookupRequest::kClassName("DirectoryLookupRequest");

    DirectoryLookupRequest::DirectoryLookupRequest(const Key& key) : KeyMessage(key, MessageType::kDirectoryLookupRequest)
    {
    }

    DirectoryLookupRequest::DirectoryLookupRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    DirectoryLookupRequest::~DirectoryLookupRequest() {}
}