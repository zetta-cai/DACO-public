#include "message/control/cooperation/bestguess/bestguess_finish_block_request.h"

namespace covered
{
    const std::string BestGuessFinishBlockRequest::kClassName("BestGuessFinishBlockRequest");

    BestGuessFinishBlockRequest::BestGuessFinishBlockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessFinishBlockRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    BestGuessFinishBlockRequest::BestGuessFinishBlockRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessFinishBlockRequest::~BestGuessFinishBlockRequest() {}
}