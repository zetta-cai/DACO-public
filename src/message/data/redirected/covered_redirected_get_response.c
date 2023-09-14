#include "message/data/redirected/covered_redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string CoveredRedirectedGetResponse::kClassName("CoveredRedirectedGetResponse");

    CoveredRedirectedGetResponse::CoveredRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueHitflagVictimsetMessage(key, value, hitflag, victim_syncset, MessageType::kCoveredRedirectedGetResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss);
    }

    CoveredRedirectedGetResponse::CoveredRedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagVictimsetMessage(msg_payload)
    {
    }

    CoveredRedirectedGetResponse::~CoveredRedirectedGetResponse() {}
}