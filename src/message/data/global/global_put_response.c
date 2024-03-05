#include "message/data/global/global_put_response.h"

namespace covered
{
    const std::string GlobalPutResponse::kClassName("GlobalPutResponse");

    GlobalPutResponse::GlobalPutResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyMessage(key, MessageType::kGlobalPutResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    GlobalPutResponse::GlobalPutResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalPutResponse::~GlobalPutResponse() {}
}