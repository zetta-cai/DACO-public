#include "message/control/cooperation/covered_placement_directory_admit_request.h"

namespace covered
{
    const std::string CoveredPlacementDirectoryAdmitRequest::kClassName("CoveredPlacementDirectoryAdmitRequest");

    CoveredPlacementDirectoryAdmitRequest::CoveredPlacementDirectoryAdmitRequest(const Key& key, const Value& value, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValueDirectoryVictimsetEdgesetMessage(key, value, directory_info, victim_syncset, edgeset, MessageType::kCoveredPlacementDirectoryAdmitRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredPlacementDirectoryAdmitRequest::CoveredPlacementDirectoryAdmitRequest(const DynamicArray& msg_payload) : KeyValueDirectoryVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredPlacementDirectoryAdmitRequest::~CoveredPlacementDirectoryAdmitRequest() {}
}