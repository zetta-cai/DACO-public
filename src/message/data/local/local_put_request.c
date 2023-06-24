#include "message/data/local/local_put_request.h"

namespace covered
{
    const std::string LocalPutRequest::kClassName("LocalPutRequest");

    LocalPutRequest::LocalPutRequest(const Key& key, const Value& value, const uint32_t& source_index) : KeyValueMessage(key, value, MessageType::kLocalPutRequest, source_index)
    {
    }

    LocalPutRequest::LocalPutRequest(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    LocalPutRequest::~LocalPutRequest() {}
}