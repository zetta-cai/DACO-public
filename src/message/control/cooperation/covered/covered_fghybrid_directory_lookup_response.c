#include "message/control/cooperation/covered/covered_fghybrid_directory_lookup_response.h"

namespace covered
{
    const std::string CoveredFghybridDirectoryLookupResponse::kClassName("CoveredFghybridDirectoryLookupResponse");

    CoveredFghybridDirectoryLookupResponse::CoveredFghybridDirectoryLookupResponse(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, edgeset, MessageType::kCoveredFghybridDirectoryLookupResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    CoveredFghybridDirectoryLookupResponse::CoveredFghybridDirectoryLookupResponse(const DynamicArray& msg_payload) : KeyWriteflagValidityDirectoryVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridDirectoryLookupResponse::~CoveredFghybridDirectoryLookupResponse() {}
}