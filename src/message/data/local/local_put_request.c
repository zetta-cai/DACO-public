#include "message/data/local/local_put_request.h"

namespace covered
{
    const std::string LocalPutRequest::kClassName("LocalPutRequest");

    LocalPutRequest::LocalPutRequest(const Key& key, const Value& value) : KeyValueMessage(key, value, MessageType::kLocalPutRequest)
    {
    }

    LocalPutRequest::LocalPutRequest(const DynamicArray& msg_payload) : KeyValueMessage(msg_payload)
    {
    }

    LocalPutRequest::~LocalPutRequest() {}
}