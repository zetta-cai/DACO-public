#include "message/control/cooperation/covered_directory_lookup_response.h"

namespace covered
{
    const std::string CoveredDirectoryLookupResponse::kClassName("CoveredDirectoryLookupResponse");

    CoveredDirectoryLookupResponse::CoveredDirectoryLookupResponse(const Key& key, const VictimSyncset& victim_syncset, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const bool& skip_propagation_latency) : KeyVictimsetWriteflagValidityDirectoryMessage(key, victim_syncset, is_being_written, is_valid_directory_exist, directory_info, MessageType::kCoveredDirectoryLookupResponse, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    CoveredDirectoryLookupResponse::CoveredDirectoryLookupResponse(const DynamicArray& msg_payload) : KeyVictimsetWriteflagValidityDirectoryMessage(msg_payload)
    {
    }

    CoveredDirectoryLookupResponse::~CoveredDirectoryLookupResponse() {}
}