#include "message/data/global/global_get_response.h"

namespace covered
{
    const std::string GlobalGetResponse::kClassName("GlobalGetResponse");

    GlobalGetResponse::GlobalGetResponse(const Key& key, const Value& value, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValueMessage(key, value, MessageType::kGlobalGetResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    GlobalGetResponse::GlobalGetResponse(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    GlobalGetResponse::~GlobalGetResponse() {}
}