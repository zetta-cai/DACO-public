#include "message/data/redirected/redirected_get_request.h"

namespace covered
{
    const std::string RedirectedGetRequest::kClassName("RedirectedGetRequest");

    RedirectedGetRequest::RedirectedGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kRedirectedGetRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    RedirectedGetRequest::RedirectedGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    RedirectedGetRequest::~RedirectedGetRequest() {}
}