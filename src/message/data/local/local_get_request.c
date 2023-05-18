#include "message/data/local/local_get_request.h"

namespace covered
{
    const std::string LocalGetRequest::kClassName("LocalGetRequest");

    LocalGetRequest::LocalGetRequest(const Key& key) : KeyMessage(key, MessageType::kLocalGetRequest)
    {
    }

    LocalGetRequest::LocalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalGetRequest::~LocalGetRequest() {}
}