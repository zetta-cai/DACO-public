#include "message/control/cooperation/covered_directory_lookup_response.h"

namespace covered
{
    const std::string CoveredDirectoryLookupResponse::kClassName("CoveredDirectoryLookupResponse");

    CoveredDirectoryLookupResponse::CoveredDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyWriteflagValidityDirectoryVictimsetMessage(key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, MessageType::kCoveredDirectoryLookupResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredDirectoryLookupResponse::CoveredDirectoryLookupResponse(const DynamicArray& msg_payload) : KeyWriteflagValidityDirectoryVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryLookupResponse::~CoveredDirectoryLookupResponse() {}
}