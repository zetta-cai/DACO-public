#include "message/data/global/global_del_request.h"

namespace covered
{
    const std::string GlobalDelRequest::kClassName("GlobalDelRequest");

    GlobalDelRequest::GlobalDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kGlobalDelRequest, source_index, source_addr, BandwidthUsage(), EventList(), extra_common_msghdr)
    {
    }

    GlobalDelRequest::GlobalDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalDelRequest::~GlobalDelRequest() {}
}