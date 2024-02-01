#include "message/control/cooperation/covered/covered_fastpath_directory_lookup_response.h"

namespace covered
{
    const std::string CoveredFastpathDirectoryLookupResponse::kClassName("CoveredFastpathDirectoryLookupResponse");

    CoveredFastpathDirectoryLookupResponse::CoveredFastpathDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const FastPathHint& fast_path_hint, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyWriteflagValidityDirectoryVictimsetFphintMessage(key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, fast_path_hint, MessageType::kCoveredFastDirectoryLookupResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    CoveredFastpathDirectoryLookupResponse::CoveredFastpathDirectoryLookupResponse(const DynamicArray& msg_payload) : KeyWriteflagValidityDirectoryVictimsetFphintMessage(msg_payload)
    {
    }

    CoveredFastpathDirectoryLookupResponse::~CoveredFastpathDirectoryLookupResponse() {}
}