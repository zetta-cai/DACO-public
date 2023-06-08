#include "message/data/redirected/redirected_get_request.h"

namespace covered
{
    const std::string RedirectedGetRequest::kClassName("RedirectedGetRequest");

    RedirectedGetRequest::RedirectedGetRequest(const Key& key) : KeyMessage(key, MessageType::kRedirectedGetRequest)
    {
    }

    RedirectedGetRequest::RedirectedGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    RedirectedGetRequest::~RedirectedGetRequest() {}
}