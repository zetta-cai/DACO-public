#include "message/data/redirected/covered/covered_redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string CoveredRedirectedGetResponse::kClassName("CoveredRedirectedGetResponse");

    CoveredRedirectedGetResponse::CoveredRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValueByteVictimsetMessage(key, value, static_cast<uint8_t>(hitflag), victim_syncset, MessageType::kCoveredRedirectedGetResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        // NOTE: hitflag could be Hitflag::kCooperativeInvalid not only for writes but also for large value sizes (in segcache/cachelib/covered)
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss || hitflag == Hitflag::kCooperativeInvalid);
    }

    CoveredRedirectedGetResponse::CoveredRedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueByteVictimsetMessage(msg_payload)
    {
    }

    CoveredRedirectedGetResponse::~CoveredRedirectedGetResponse() {}

    Hitflag CoveredRedirectedGetResponse::getHitflag() const
    {
        return static_cast<Hitflag>(getByte_());
    }
}