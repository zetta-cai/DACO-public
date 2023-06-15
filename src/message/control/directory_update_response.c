#include "message/control/directory_update_response.h"

namespace covered
{
    const std::string DirectoryUpdateResponse::kClassName("DirectoryUpdateResponse");

    DirectoryUpdateResponse::DirectoryUpdateResponse(const Key& key, const bool& is_being_written) : KeyWriteflagMessage(key, is_being_written, MessageType::kDirectoryUpdateResponse)
    {
    }

    DirectoryUpdateResponse::DirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyWriteflagMessage(msg_payload)
    {
    }

    DirectoryUpdateResponse::~DirectoryUpdateResponse() {}
}