#include "message/control/cooperation/directory_lookup_response.h"

namespace covered
{
    const std::string DirectoryLookupResponse::kClassName("DirectoryLookupResponse");

    DirectoryLookupResponse::DirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : KeyWriteflagValidityDirectoryMessage(key, is_being_written, is_valid_directory_exist, directory_info, MessageType::kDirectoryLookupResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    DirectoryLookupResponse::DirectoryLookupResponse(const DynamicArray& msg_payload) : KeyWriteflagValidityDirectoryMessage(msg_payload)
    {
    }

    DirectoryLookupResponse::~DirectoryLookupResponse() {}
}