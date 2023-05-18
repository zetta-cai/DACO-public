#include "message/data/global/global_get_request.h"

namespace covered
{
    const std::string GlobalGetRequest::kClassName("GlobalGetRequest");

    GlobalGetRequest::GlobalGetRequest(const Key& key) : KeyMessage(key, MessageType::kGlobalGetRequest)
    {
    }

    GlobalGetRequest::GlobalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    GlobalGetRequest::~GlobalGetRequest() {}
}