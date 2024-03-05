#include "message/control/cooperation/bestguess/bestguess_invalidation_response.h"

namespace covered
{
    const std::string BestGuessInvalidationResponse::kClassName("BestGuessInvalidationResponse");

    BestGuessInvalidationResponse::BestGuessInvalidationResponse(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessInvalidationResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    BestGuessInvalidationResponse::BestGuessInvalidationResponse(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessInvalidationResponse::~BestGuessInvalidationResponse() {}
}