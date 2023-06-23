#include "message/control/acquire_writelock_response.h"

namespace covered
{
    const std::string AcquireWritelockResponse::kClassName("AcquireWritelockResponse");

    AcquireWritelockResponse::AcquireWritelockResponse(const Key& key, const bool& is_successful) : KeyByteMessage(key, static_cast<uint8_t>(is_successful), MessageType::kAcquireWritelockResponse)
    {
    }

    AcquireWritelockResponse::AcquireWritelockResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    AcquireWritelockResponse::~AcquireWritelockResponse() {}

    bool AcquireWritelockResponse::isSuccessful() const
    {
        uint8_t byte = getByte_();
        bool is_successful = static_cast<bool>(byte);
        return is_successful;
    }
}