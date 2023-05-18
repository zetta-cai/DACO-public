#include "message/data/local/local_del_response.h"

namespace covered
{
    const std::string LocalDelResponse::kClassName("LocalDelResponse");

    LocalDelResponse::LocalDelResponse(const Key& key) : KeyMessage(key, MessageType::kLocalDelResponse)
    {
    }

    LocalDelResponse::LocalDelResponse(const DynamicArray& msg_payload) : MessageBase(msg_payload)
    {
    }

    LocalDelResponse::~LocalDelResponse() {}
}