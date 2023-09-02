#include "message/control/cooperation/covered_directory_lookup_request.h"

namespace covered
{
    const std::string CoveredDirectoryLookupRequest::kClassName("CoveredDirectoryLookupRequest");

    CoveredDirectoryLookupRequest::CoveredDirectoryLookupRequest(const Key& key, const bool& is_tracked, const Popularity& local_uncached_popularity, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyTrackflagPopularityVictimsetMessage(key, is_tracked, local_uncached_popularity, victim_syncset, MessageType::kCoveredDirectoryLookupRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    CoveredDirectoryLookupRequest::CoveredDirectoryLookupRequest(const DynamicArray& msg_payload) : KeyTrackflagPopularityVictimsetMessage(msg_payload)
    {
    }

    CoveredDirectoryLookupRequest::~CoveredDirectoryLookupRequest() {}
}