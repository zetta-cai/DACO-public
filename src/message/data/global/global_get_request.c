#include "message/data/global/global_get_request.h"

namespace covered
{
    const std::string GlobalGetRequest::kClassName("GlobalGetRequest");

    GlobalGetRequest::GlobalGetRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kGlobalGetRequest, source_index)
    {
    }

    GlobalGetRequest::GlobalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalGetRequest::~GlobalGetRequest() {}
}