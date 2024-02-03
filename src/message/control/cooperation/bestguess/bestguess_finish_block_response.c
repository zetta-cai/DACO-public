#include "message/control/cooperation/bestguess/bestguess_finish_block_response.h"

namespace covered
{
    const std::string BestGuessFinishBlockResponse::kClassName("BestGuessFinishBlockResponse");

    BestGuessFinishBlockResponse::BestGuessFinishBlockResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessFinishBlockResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
    }

    BestGuessFinishBlockResponse::BestGuessFinishBlockResponse(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessFinishBlockResponse::~BestGuessFinishBlockResponse() {}
}