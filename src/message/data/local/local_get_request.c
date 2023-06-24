#include "message/data/local/local_get_request.h"

namespace covered
{
    const std::string LocalGetRequest::kClassName("LocalGetRequest");

    LocalGetRequest::LocalGetRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kLocalGetRequest, source_index)
    {
    }

    LocalGetRequest::LocalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalGetRequest::~LocalGetRequest() {}
}