#include "message/data/local/local_get_request.h"

namespace covered
{
    const std::string LocalGetRequest::kClassName("LocalGetRequest");

    LocalGetRequest::LocalGetRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& skip_propagation_latency) : KeyMessage(key, MessageType::kLocalGetRequest, source_index, source_addr, EventList(), skip_propagation_latency)
    {
    }

    LocalGetRequest::LocalGetRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalGetRequest::~LocalGetRequest() {}
}