#include "message/data/global/global_del_response.h"

namespace covered
{
    const std::string GlobalDelResponse::kClassName("GlobalDelResponse");

    GlobalDelResponse::GlobalDelResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kGlobalDelResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    GlobalDelResponse::GlobalDelResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelResponse::~GlobalDelResponse() {}
}