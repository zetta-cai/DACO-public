#include "message/data/global/global_get_response.h"

namespace covered
{
    const std::string GlobalGetResponse::kClassName("GlobalGetResponse");

    GlobalGetResponse::GlobalGetResponse(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyValueMessage(key, value, MessageType::kGlobalGetResponse, source_index, source_addr, event_list)
    {
    }

    GlobalGetResponse::GlobalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    GlobalGetResponse::~GlobalGetResponse() {}
}