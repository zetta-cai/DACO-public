#include "message/data/local/local_get_response.h"

namespace covered
{
    const std::string LocalGetResponse::kClassName("LocalGetResponse");

    LocalGetResponse::LocalGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyValueHitflagMessage(key, value, hitflag, MessageType::kLocalGetResponse, source_index, source_addr, event_list)
    {
    }

    LocalGetResponse::LocalGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagMessage(msg_payload)
    {
    }

    LocalGetResponse::~LocalGetResponse() {}
}