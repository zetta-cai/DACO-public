#include "message/data/redirected/redirected_get_response.h"

#include <assert.h>

namespace covered
{
    const std::string RedirectedGetResponse::kClassName("RedirectedGetResponse");

    RedirectedGetResponse::RedirectedGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyValueHitflagMessage(key, value, hitflag, MessageType::kRedirectedGetResponse, source_index, source_addr, event_list)
    {
        assert(hitflag == Hitflag::kCooperativeHit || hitflag == Hitflag::kGlobalMiss);
    }

    RedirectedGetResponse::RedirectedGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagMessage(msg_payload)
    {
    }

    RedirectedGetResponse::~RedirectedGetResponse() {}
}