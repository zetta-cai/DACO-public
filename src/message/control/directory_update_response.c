#include "message/control/directory_update_response.h"

namespace covered
{
    const std::string DirectoryUpdateResponse::kClassName("DirectoryUpdateResponse");

    DirectoryUpdateResponse::DirectoryUpdateResponse(const Key& key, const bool& is_being_written) : KeyByteMessage(key, static_cast<uint8_t>(is_being_written), MessageType::kDirectoryUpdateResponse)
    {
    }

    DirectoryUpdateResponse::DirectoryUpdateResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    DirectoryUpdateResponse::~DirectoryUpdateResponse() {}

    bool DirectoryUpdateResponse::isBeingWritten() const
    {
        uint8_t byte = getByte_();
        bool is_being_written = static_cast<bool>(byte);
        return is_being_written;
    }
}