#include "message/data/redirected/covered/covered_bgfetch_redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string CoveredBgfetchRedirectedGetResponse::kClassName("CoveredBgfetchRedirectedGetResponse");

    CoveredBgfetchRedirectedGetResponse::CoveredBgfetchRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueHitflagVictimsetEdgesetMessage(key, value, hitflag, victim_syncset, edgeset, MessageType::kCoveredBgfetchRedirectedGetResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        // NOTE: hitflag could be Hitflag::kCooperativeInvalid not only for writes but also for large value sizes (in segcache/cachelib/covered)
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss || hitflag == Hitflag::kCooperativeInvalid);
    }

    CoveredBgfetchRedirectedGetResponse::CoveredBgfetchRedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagVictimsetEdgesetMessage(msg_payload)
    {
    }

    CoveredBgfetchRedirectedGetResponse::~CoveredBgfetchRedirectedGetResponse() {}
}