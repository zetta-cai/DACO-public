#include "message/control/cooperation/covered/covered_fghybrid_directory_admit_request.h"

namespace covered
{
    const std::string CoveredFghybridDirectoryAdmitRequest::kClassName("CoveredFghybridDirectoryAdmitRequest");

    CoveredFghybridDirectoryAdmitRequest::CoveredFghybridDirectoryAdmitRequest(const Key& key, const Value& value, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueDirectoryVictimsetEdgesetMessage(key, value, directory_info, victim_syncset, edgeset, MessageType::kCoveredFghybridDirectoryAdmitRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredFghybridDirectoryAdmitRequest::CoveredFghybridDirectoryAdmitRequest(const DynamicArray& msg_payload) : KeyValueDirectoryVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredFghybridDirectoryAdmitRequest::~CoveredFghybridDirectoryAdmitRequest() {}
}