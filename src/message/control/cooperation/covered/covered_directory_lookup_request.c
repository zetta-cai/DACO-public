#include "message/control/cooperation/covered/covered_directory_lookup_request.h"

namespace covered
{
    const std::string CoveredDirectoryLookupRequest::kClassName("CoveredDirectoryLookupRequest");

    CoveredDirectoryLookupRequest::CoveredDirectoryLookupRequest(const Key& key, const CollectedPopularity& collected_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyCollectpopVictimsetMessage(key, collected_popularity, victim_syncset, MessageType::kCoveredDirectoryLookupRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    CoveredDirectoryLookupRequest::CoveredDirectoryLookupRequest(const DynamicArray& msg_payload) : KeyCollectpopVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryLookupRequest::~CoveredDirectoryLookupRequest() {}
}