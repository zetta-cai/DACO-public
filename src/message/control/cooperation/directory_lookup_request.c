#include "message/control/cooperation/directory_lookup_request.h"

namespace covered
{
    const std::string DirectoryLookupRequest::kClassName("DirectoryLookupRequest");

    DirectoryLookupRequest::DirectoryLookupRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kDirectoryLookupRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    DirectoryLookupRequest::DirectoryLookupRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    DirectoryLookupRequest::~DirectoryLookupRequest() {}
}