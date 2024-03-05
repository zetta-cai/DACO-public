#include "message/control/cooperation/bestguess/bestguess_invalidation_request.h"

namespace covered
{
    const std::string BestGuessInvalidationRequest::kClassName("BestGuessInvalidationRequest");

    BestGuessInvalidationRequest::BestGuessInvalidationRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessInvalidationRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    BestGuessInvalidationRequest::BestGuessInvalidationRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessInvalidationRequest::~BestGuessInvalidationRequest() {}
}