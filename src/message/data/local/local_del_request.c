#include "message/data/local/local_del_request.h"

namespace covered
{
    const std::string LocalDelRequest::kClassName("LocalDelRequest");

    LocalDelRequest::LocalDelRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr) : KeyMessage(key, MessageType::kLocalDelRequest, source_index, source_addr)
    {
    }

    LocalDelRequest::LocalDelRequest(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalDelRequest::~LocalDelRequest() {}
}