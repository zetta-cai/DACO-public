#include "message/control/cooperation/covered_directory_update_request.h"

namespace covered
{
    const std::string CoveredDirectoryUpdateRequest::kClassName("CoveredDirectoryUpdateRequest");

    CoveredDirectoryUpdateRequest::CoveredDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValidityDirectoryCollectpopVictimsetMessage(key, is_valid_directory_exist, directory_info, victim_syncset, MessageType::kCoveredDirectoryUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredDirectoryUpdateRequest::CoveredDirectoryUpdateRequest(const Key& key, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyValidityDirectoryCollectpopVictimsetMessage(key, is_valid_directory_exist, directory_info, collected_popularity, victim_syncset, MessageType::kCoveredDirectoryUpdateRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredDirectoryUpdateRequest::CoveredDirectoryUpdateRequest(const DynamicArray& msg_payload) : KeyValidityDirectoryCollectpopVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryUpdateRequest::~CoveredDirectoryUpdateRequest() {}
}