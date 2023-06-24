#include "message/data/local/local_del_request.h"

namespace covered
{
    const std::string LocalDelRequest::kClassName("LocalDelRequest");

    LocalDelRequest::LocalDelRequest(const Key& key, const uint32_t& source_index) : KeyMessage(key, MessageType::kLocalDelRequest, source_index)
    {
    }

    LocalDelRequest::LocalDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalDelRequest::~LocalDelRequest() {}
}