#include "message/data/global/global_get_request.h"

namespace covered
{
    const std::string GlobalGetRequest::kClassName("GlobalGetRequest");

    GlobalGetRequest::GlobalGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kGlobalGetRequest, source_index, source_addr, EventList())
    {
    }

    GlobalGetRequest::GlobalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalGetRequest::~GlobalGetRequest() {}
}