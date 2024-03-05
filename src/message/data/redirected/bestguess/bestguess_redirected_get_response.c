#include "message/data/redirected/bestguess/bestguess_redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string BestGuessRedirectedGetResponse::kClassName("BestGuessRedirectedGetResponse");

    BestGuessRedirectedGetResponse::BestGuessRedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const BestGuessSyncinfo& syncinfo, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValueByteSyncinfoMessage(key, value, static_cast<uint8_t>(hitflag), syncinfo, MessageType::kBestGuessRedirectedGetResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
        // NOTE: hitflag could be Hitflag::kCooperativeInvalid not only for writes but also for large value sizes (in segcache/cachelib/covered)
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss || hitflag == Hitflag::kCooperativeInvalid);
    }

    BestGuessRedirectedGetResponse::BestGuessRedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueByteSyncinfoMessage(msg_payload)
    {
    }

    BestGuessRedirectedGetResponse::~BestGuessRedirectedGetResponse() {}

    Hitflag BestGuessRedirectedGetResponse::getHitflag() const
    {
        uint8_t byte = getByte_();
        return static_cast<Hitflag>(byte);
    }
}