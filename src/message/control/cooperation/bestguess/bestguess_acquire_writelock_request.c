#include "message/control/cooperation/bestguess/bestguess_acquire_writelock_request.h"

namespace covered
{
    const std::string BestGuessAcquireWritelockRequest::kClassName("BestGuessAcquireWritelockRequest");

    BestGuessAcquireWritelockRequest::BestGuessAcquireWritelockRequest(const Key& key, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeySyncinfoMessage(key, syncinfo, MessageType::kBestGuessAcquireWritelockRequest, source_index, source_addr, BandwidthUsage(), EventList(), skip_propagation_latency)
    {
    }

    BestGuessAcquireWritelockRequest::BestGuessAcquireWritelockRequest(const DynamicArray& msg_payload) : KeySyncinfoMessage(msg_payload)
    {
    }

    BestGuessAcquireWritelockRequest::~BestGuessAcquireWritelockRequest() {}
}