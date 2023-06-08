#include "message/control/directory_update_response.h"

namespace covered
{
    const std::string DirectoryUpdateResponse::kClassName("DirectoryUpdateResponse");

    DirectoryUpdateResponse::DirectoryUpdateResponse(const Key& key) : KeyMessage(key, MessageType::kDirectoryUpdateResponse)
    {
    }

    DirectoryUpdateResponse::DirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    DirectoryUpdateResponse::~DirectoryUpdateResponse() {}
}