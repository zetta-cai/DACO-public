#include "message/control/cooperation/bestguess/bestguess_directory_lookup_request.h"

namespace covered
{
    const std::string BestGuessDirectoryLookupRequest::kClassName("BestGuessDirectoryLookupRequest");

    BestGuessDirectoryLookupRequest::BestGuessDirectoryLookupRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessDirectoryLookupRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    BestGuessDirectoryLookupRequest::BestGuessDirectoryLookupRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessDirectoryLookupRequest::~BestGuessDirectoryLookupRequest() {}
}