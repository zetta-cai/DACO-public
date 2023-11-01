#include "message/data/redirected/redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string RedirectedGetResponse::kClassName("RedirectedGetResponse");

    RedirectedGetResponse::RedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyValueHitflagMessage(key, value, hitflag, MessageType::kRedirectedGetResponse, source_index, source_addr, bandwidth_usage, event_list, skip_propagation_latency)
    {
        // NOTE: hitflag could be Hitflag::kCooperativeInvalid not only for writes but also for large value sizes (in segcache/cachelib/covered)
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss || hitflag == Hitflag::kCooperativeInvalid);
    }

    RedirectedGetResponse::RedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagMessage(msg_payload)
    {
    }

    RedirectedGetResponse::~RedirectedGetResponse() {}
}